cmake_minimum_required(VERSION 3.12)
project(ssh_tcp_reverse_shell_cpp)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        /usr/local/Cellar/libssh/0.8.6/include/
        utils/
)

add_library(libssh SHARED IMPORTED GLOBAL)
set_property(TARGET libssh PROPERTY IMPORTED_LOCATION /usr/local/Cellar/libssh/0.8.6/lib/libssh.dylib)

add_library(libutils
        utils/utils.h
        utils/utils.cpp
)

# PAYLOAD

add_executable(payload
        payload/main.cpp
        payload/Payload.h
        payload/Payload.cpp
)
target_link_libraries(payload PRIVATE libssh libutils)

# C2

add_executable(c2
        c2/main.cpp
        c2/C2Server.h
        c2/C2Server.cpp
)
target_link_libraries(c2 PRIVATE libssh libutils)

# C version payload
add_executable(c-version-payload
        c-version-payload/main.c
        c-version-payload/payload.h
        c-version-payload/payload.c
        c-version-payload/utils.c
        c-version-payload/utils.h
)
target_link_libraries(c-version-payload PRIVATE libssh)