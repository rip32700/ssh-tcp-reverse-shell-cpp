#include <iostream>
#include "libssh/libssh.h"
#include "Payload.h"

#include "utils.h"

int main()
{
    Payload payload;
    payload.connect();

    return 0;
}