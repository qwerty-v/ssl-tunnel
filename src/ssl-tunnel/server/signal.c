#include <ssl-tunnel/lib/err.h>

#include <signal.h>
#include <stdbool.h>
#include <stdatomic.h>

static volatile atomic_bool _signal_flag;

static void _signal_handler(int unused) {
    (void) unused;
    atomic_store(&_signal_flag, true);
}

bool signal_read_flag() {
    return atomic_load(&_signal_flag);
}

void signal_init() {
    atomic_init(&_signal_flag, false);

    signal(SIGHUP, _signal_handler);
    signal(SIGINT, _signal_handler);
    signal(SIGQUIT, _signal_handler);
    signal(SIGKILL, _signal_handler);
    signal(SIGTERM, _signal_handler);
}
