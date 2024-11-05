FROM ubuntu:24.04 as builder
WORKDIR /app

RUN apt-get update && \
    apt-get -y install gcc make

COPY . .

RUN make

FROM builder as test

RUN make test

RUN ./bin/unit_test_arrays
RUN ./bin/unit_test_memory

FROM alpine:3.19
WORKDIR /app

RUN apk add --update bash iptables libc6-compat

COPY --from=builder /app/bin/ssl-tunnel-server .
COPY ./scripts/docker_entrypoint.sh .
COPY ./config.yaml .

CMD ["./docker_entrypoint.sh"]