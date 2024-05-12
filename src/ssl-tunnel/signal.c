#include <signal.h>
#include <assert.h>

static volatile int *sig_received = 0;

void signal_handler(int _) {
    if (!sig_received) {
        assert(0);
    }

    *sig_received = 1;
}

void signal_init(volatile int *s) {
    sig_received = s;

    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);
}
