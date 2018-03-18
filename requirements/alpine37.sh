#!/bin/sh

set -xe

if [ "$(grep -c 'Alpine Linux v3.7' /etc/os-release)" == "0" ]; then
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
    g++ \
    git \
    libtool \
    make \
    netcat-openbsd \
    valgrind \
    vim
