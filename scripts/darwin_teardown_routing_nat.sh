#!/bin/sh
# usage: sudo ./darwin_teardown_routing_nat.sh

# disable routing
sysctl -w net.inet.ip.forwarding=0

# teardown nat
pfctl -d