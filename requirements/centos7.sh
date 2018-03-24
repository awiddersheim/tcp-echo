#!/bin/sh

set -xe

. /etc/os-release

if [ "${ID}" != "centos" ] || [ "${VERSION_ID}" != "7" ]; then
    exit 0
fi

yum install -y \
    automake \
    clang \
    clang-analyzer \
    cmake \
    gcc \
    gcc-c++ \
    git \
    libtool \
    make \
    nc \
    telnet \
    valgrind \
    vim
