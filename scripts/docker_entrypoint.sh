#!/bin/bash

mkdir -p /dev/net
mknod /dev/net/tun c 10 200

# sysctl checks
if [ "$(</proc/sys/net/ipv4/ip_forward)" != "1" ]; then
  echo "ip forwarding is disabled, please try running docker with '--sysctl net.ipv4.ip_forward=1'"
fi

./preup.sh

exec ./ssl-tunnel