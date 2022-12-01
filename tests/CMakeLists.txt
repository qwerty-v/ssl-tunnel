project(tests C)

set(CMAKE_C_STANDARD 99)

add_executable(test1 unit/tun.c
        ../../src/ssl_tunnel/tun.c
        ../../src/ssl_tunnel/fd.c)

add_executable(proto_test unit/proto.c
        ../../src/ssl_tunnel/proto.c
        ../../src/ssl_tunnel/buffer.c)

add_test(NAME Test1 COMMAND test1)