#!/bin/sh

set -xe

. /etc/os-release

if [ "${ID}" != "alpine" ] || [ "${VERSION_ID}" != "3.7.0" ]; then
    exit 0
fi

apk add --update \
    autoconf \
    automake \
    bash \
    busybox-extras \
    clang \
    clang-analyzer \
    cmake \
    file \
    gcc \
    git \
    libtool \
    make \
    musl-dev \
    netcat-openbsd \
    valgrind \
    vim
