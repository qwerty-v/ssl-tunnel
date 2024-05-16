#include <ssl-tunnel/server.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
    err_t err;

    if (!ERR_OK(err = server_main(argc, argv))) {
        printf("error: %s\n", err.msg);
        return err;
    }

    return 0;
}