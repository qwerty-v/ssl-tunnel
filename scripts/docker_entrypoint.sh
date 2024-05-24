#!/bin/bash

mkdir -p /dev/net
mknod /dev/net/tun c 10 200

# routing setup
if [ "$(</proc/sys/net/ipv4/ip_forward)" != "1" ]; then
  echo "ip forwarding is disabled, please try running docker with '--sysctl net.ipv4.ip_forward=1'"
fi

# nat setup
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

exec ./ssl-tunnel --cfg config.yaml