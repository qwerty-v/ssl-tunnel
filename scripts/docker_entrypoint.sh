#!/bin/bash

mkdir -p /dev/net
mknod /dev/net/tun c 10 200

# sysctl checks
if [ "$(</proc/sys/net/ipv4/ip_forward)" != "1" ]; then
  echo "ip forwarding is disabled, please try running docker with '--sysctl net.ipv4.ip_forward=1'"
fi

# device setup
ip tuntap add dev tun0 mode tun
ip addr add 10.8.0.1/24 dev tun0
ip link set tun0 up

# nat setup
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

exec ./ssl-tunnel