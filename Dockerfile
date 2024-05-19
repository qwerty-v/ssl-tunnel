FROM ubuntu:24.04 as builder
WORKDIR /app

RUN apt-get update && \
    apt-get -y install gcc make

COPY . .

RUN make

FROM alpine:3.19
WORKDIR /app

COPY --from=builder /app/bin/ssl-tunnel .
COPY ./config.yaml .

RUN apk add libc6-compat

CMD ["/app/ssl-tunnel", "--cfg", "/app/config.yaml"]