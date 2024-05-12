#include <ssl-tunnel/server.h>

int main(int argc, char *argv[]) {
    err_t err;

    if (!ERR_OK(err = server_main(argc, argv))) {
        errors_print(err);
        return err;
    }

    return 0;
}