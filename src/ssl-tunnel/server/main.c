#include <ssl-tunnel/server/server.h>

#include <stdio.h> // printf
#include <errno.h> // errno

int main(int argc, char *argv[]) {
    err_t err = server_main(argc, argv);
    if (!ERROR_OK(err)) {
        printf("Error: %s\n", err.msg);
        return errno;
    }

    return 0;
}