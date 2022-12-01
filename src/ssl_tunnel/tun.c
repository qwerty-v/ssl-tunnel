#include "tun.h"
#include "fd.h"

#if __APPLE__

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

int tun_open() {
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        return -1;
    }

    struct ctl_info ctlInfo;
    memset(&ctlInfo, 0, sizeof(ctlInfo));
    if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >= sizeof(ctlInfo.ctl_name)) {
        return -1;
    }

    if (ioctl(fd, CTLIOCGINFO, &ctlInfo) < 0) {
        close(fd);
        return -1;
    }

    struct sockaddr_ctl sc;
    sc.sc_id = ctlInfo.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_unit = 4;	// utun3

    // If the connect is successful, a tun%d device will be created, where "%d"
    // is our unit number -1

    if (connect(fd, (struct sockaddr *) &sc, sizeof(sc)) < 0) {
        close(fd);
        return -1;
    }

    if (fd_set_nonblocking(fd) < 0) {
        return -1;
    }

    if (fd_set_cloexec(fd) < 0) {
        return -1;
    }

    return fd;
}

#elif __linux__

#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

int tun_open() {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_NO_PI | IFF_TUN;
    strncopy(ifr.ifr_name, "tun0", IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        return -1;
    }

    if (fd_set_nonblocking(fd) < 0) {
        return -1;
    }

    if (fd_set_cloexec(fd) < 0) {
        return -1;
    }

    return fd;
}

#else
#error "not implemented"
#endif