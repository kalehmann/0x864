FROM alpine:latest
ARG UID=1000
ARG GID=1000

RUN mkdir /0x864
WORKDIR /0x864

RUN apk add --no-cache bash gcc make musl-dev nasm

USER ${UID}:${GID}
