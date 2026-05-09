FROM alpine:latest AS nasm_build
ARG NASM_VERSION="3.02rc7"

RUN mkdir -p /nasm \
    && cd /nasm \
    && wget -O nasm.tar.gz https://github.com/netwide-assembler/nasm/archive/refs/tags/nasm-${NASM_VERSION}.tar.gz \
    && tar -xf nasm.tar.gz --strip-components=1 \
    && rm -rf nasm.tar.gz

WORKDIR /nasm

RUN apk add --no-cache \
    asciidoc \
    autoconf \
    automake \
    gcc \
    make \
    musl-dev \
    xmlto

RUN sh autogen.sh \
    && sh configure LDFLAGS=-static

RUN make \
    && make manpages \
    && make install

FROM alpine:latest
ARG UID=1000
ARG GID=1000

RUN mkdir /0x864
WORKDIR /0x864

RUN apk add --no-cache bash binutils make musl-dev tcc tcc-libs-static

COPY --from=nasm_build /usr/local/bin/nasm /usr/local/bin/nasm

USER ${UID}:${GID}
