#!/bin/sh
# usage: sudo ./darwin_setup_routing_nat.sh

# https://openvpn.net/cloud-docs/enabling-routing-nat-on-macos/

# enable routing
sysctl -w net.inet.ip.forwarding=1

# setup nat
pfctl -d # disables pfctl
pfctl -F nat # flushes nat pfctl rules
echo "nat on en0 from 10.8.0.0/24 to any -> (en0)" | pfctl -ef - # starts pfctl and loads the nat rule