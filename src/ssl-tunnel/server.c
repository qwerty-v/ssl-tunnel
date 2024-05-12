#include <ssl-tunnel/signal.h>
#include <ssl-tunnel/memory.h>
#include <ssl-tunnel/errors.h>

typedef struct {
    volatile int running;
} server;

err_t server_main(int argc, char *argv[]) {
    err_t err;

    mem_scope *m;
    if (!ERR_OK(err = mem_alloc_mem_scope(&m))) {
        return err;
    }

    if (!ERR_OK(err = mem_scope_init(m))) {
        mem_destroy_mem_scope(m);
        return err;
    }

    server *srv;
    if (!ERR_OK(err = mem_scope_alloc(m, (void **) &srv, sizeof(server)))) {
        mem_destroy_mem_scope(m);
        return err;
    }

    signal_init(&srv->running);

    mem_destroy_mem_scope(m);
    return ERROR_OK;
}