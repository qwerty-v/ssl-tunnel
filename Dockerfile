FROM ubuntu:24.04 as builder
WORKDIR /app

RUN apt-get update && \
    apt-get -y install gcc make

COPY . .

RUN make

FROM alpine:3.19
WORKDIR /app

RUN apk add --update bash iptables libc6-compat

COPY --from=builder /app/bin/ssl-tunnel .
COPY ./scripts/docker_entrypoint.sh .
COPY ./config.yaml .

CMD ["./docker_entrypoint.sh"]