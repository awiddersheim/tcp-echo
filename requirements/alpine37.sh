#!/bin/sh

set -xe

source /etc/os-release

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
    g++ \
    git \
    libtool \
    make \
    netcat-openbsd \
    valgrind \
    vim
