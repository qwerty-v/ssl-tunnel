#!/bin/sh
# usage: sudo ./linux_setup_routing_nat.sh

# enable routing
sysctl -w net.ipv4.ip_forward=1

# setup nat
iptables -t nat -A POSTROUTING -o enp0s3 -j MASQUERADE