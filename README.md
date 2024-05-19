# ssl-tunnel

`ssl-tunnel` is a VPN deamon written in C
inspired by OpenVPN

See OpenVPN GitHub [here](https://github.com/OpenVPN/openvpn)
and OpenVPN protocol overview [here](https://build.openvpn.net/doxygen/network_protocol.html)

## Current limitations
no-ssl implementation

## Run
    make build
    ./bin/ssl-tunnel --cfg <config_path>.yaml