cmake_minimum_required(VERSION 3.12)
project(ssh_tcp_reverse_shell_cpp)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        /usr/local/Cellar/libssh/0.8.6/include/
)

add_library(libssh SHARED IMPORTED GLOBAL)
set_property(
        TARGET libssh PROPERTY IMPORTED_LOCATION /usr/local/Cellar/libssh/0.8.6/lib/libssh.dylib
)

# PAYLOAD

add_executable(payload
        payload/main.cpp
        payload/Payload.h
        payload/Payload.cpp
        payload/utils.h
        payload/utils.cpp
)
target_link_libraries(payload
        libssh
)

# C2

add_executable(c2
        c2/main.cpp
        c2/C2Server.h
        c2/C2Server.cpp
        c2/utils.h
        c2/utils.cpp
        )

target_link_libraries(c2
        libssh
)