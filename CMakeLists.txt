cmake_minimum_required(VERSION 3.12)
project(ssh_tcp_reverse_shell_cpp)

set(CMAKE_CXX_STANDARD 17)

INCLUDE_DIRECTORIES(/usr/local/Cellar/libssh/0.8.6/include/)

add_executable(payload payload/main.cpp payload/Payload.h payload/Payload.cpp)
add_executable(c2 c2/main.cpp c2/C2.h c2/C2.cpp)