project(ssl_tunnel)

#set(OPENSSL_ROOT_DIR /opt/homebrew/Cellar/openssl@3/3.0.3)
#set(OPENSSL_LIBRARIES /opt/homebrew/Cellar/openssl@3/3.0.3/lib)
#
#find_package(OpenSSL REQUIRED)

add_executable(ssl_tunnel
        ssl_tunnel/ssl_tunnel.c ssl_tunnel/ssl_tunnel.h
        ssl_tunnel/socket.c ssl_tunnel/socket.h
        ssl_tunnel/poll.c ssl_tunnel/poll.h
        ssl_tunnel/tun.c ssl_tunnel/tun.h
        ssl_tunnel/fd.c ssl_tunnel/fd.h
        ssl_tunnel/buffer.c ssl_tunnel/buffer.h
        ssl_tunnel/proto.h ssl_tunnel/proto.c
        ssl_tunnel/target.h
        ssl_tunnel/ifconfig.c ssl_tunnel/ifconfig.h)