#include <ssl-tunnel/signal.h>
#include <ssl-tunnel/memory.h>
#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/flag.h>

#include <stdio.h>

typedef struct {
    volatile bool sig_received;
} server;

err_t server_main(int argc, char *argv[]) {
    alloc_pool_t p;
    alloc_pool_init(&p);

    err_t err;
    char *cfg_path;
    if (!ERR_OK(err = flag_parse(argc, argv, &cfg_path))) {
        return err;
    }

    printf("%s %ld\n", cfg_path, strlen(cfg_path));

    server srv;

    signal_init(&srv.sig_received);

    alloc_pool_free(&p);
    return ERROR_OK;
}