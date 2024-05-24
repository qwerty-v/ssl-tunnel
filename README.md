# ssl-tunnel

`ssl-tunnel` is a VPN deamon written in C
inspired by OpenVPN

See OpenVPN GitHub [here](https://github.com/OpenVPN/openvpn)
and OpenVPN protocol overview [here](https://build.openvpn.net/doxygen/network_protocol.html)

## Current limitations
one client only

udp only

tun only

no-ssl implementation

## Run
    $ docker build -t ssl-tunnel:0.0.1 .
    $ docker run -d --cap-add=NET_ADMIN -p 1026:1026/udp ssl-tunnel:0.0.1

## Test
    $ docker build -t ssl-tunnel:0.0.1 . --target=test