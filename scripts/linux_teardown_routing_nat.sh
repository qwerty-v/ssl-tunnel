#!/bin/sh
# usage: sudo ./linux_teardown_routing_nat.sh

# disable routing
sysctl -w net.ipv4.ip_forward=0

# teardown nat
iptables -t nat -D POSTROUTING -o enp0s3 -j MASQUERADE