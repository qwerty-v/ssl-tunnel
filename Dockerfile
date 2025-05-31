FROM ubuntu:24.04 AS builder
WORKDIR /app

RUN apt-get update && \
    apt-get -y install gcc make

COPY . .

RUN make

FROM builder AS test

RUN make test

CMD ./bin/unit_alloc; ./bin/unit_slice; ./bin/unit_memscope; ./bin/unit_deque

FROM alpine:3.19
WORKDIR /app

RUN apk add --update --no-cache \
    bash \
    iproute2 \
    iptables \
    libc6-compat

COPY --from=builder /app/bin/ssl-tunnel ./ssl-tunnel
COPY ./scripts/docker_entrypoint.sh .
COPY ./preup.sh .
COPY ./config.yaml /etc/ssl-tunnel/cfg.yaml

CMD ["./docker_entrypoint.sh"]