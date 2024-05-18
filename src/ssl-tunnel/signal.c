#include <ssl-tunnel/errors.h>

#include <signal.h>
#include <stdbool.h>

static volatile bool *sig_received = 0;

void signal_handler(int _) {
    if (!sig_received) {
        panicf("invalid pointer to a state variable");
    }

    *sig_received = true;
}

void signal_init(volatile bool *s) {
    *s = false;
    sig_received = s;

    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);
}
