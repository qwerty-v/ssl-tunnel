#include "target.h"

#include <stdlib.h>
#include <printf.h>

#ifdef TARGET_APPLE

int ifconfig_device_up() {
    char cmd[] = "ifconfig utun3 10.8.0.1 10.8.0.2 mtu 1500 netmask 255.255.255.255 up";
    printf("%s\n", cmd);
    return system(cmd);
}

#elifdef TARGET_LINUX

int ifconfig_device_up() {
    return system("ifconfig tun0 10.8.0.1/24 mtu 1500 up");
}

#else
#error "not implemented"
#endif