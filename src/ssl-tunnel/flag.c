#include <ssl-tunnel/errors.h>

#include <getopt.h>

const err_t ERROR_INVALID_USAGE = {
        .ok = false,
        .msg = "invalid usage"
};

err_t flag_parse(int argc, char *argv[], char **cfg_path) {
    static struct option long_options[] = {
            {"cfg", required_argument, 0, 'c'},
            {0,     0,                 0, 0}
    };

    int opt = getopt_long(argc, argv, "c:", long_options, 0);
    if (opt != 'c') {
        return ERROR_INVALID_USAGE;
    }

    *cfg_path = optarg;
    return ERROR_OK;
}