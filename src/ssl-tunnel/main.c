#include <ssl-tunnel/server.h>
#include <ssl-tunnel/flag.h>

#include <stdio.h>
#include <errno.h>

void print_usage(char *executable) {
    printf("Usage: %s --cfg <config_file>.yaml\n", executable);
}

int main(int argc, char *argv[]) {
    err_t err;

    if (!ERR_OK(err = server_main(argc, argv))) {
        if (ERR_IS(err, ERROR_INVALID_USAGE)) {
            print_usage(argv[0]);
        }

        printf("error: %s\n", err.msg);
        return errno;
    }

    return 0;
}