#!/bin/bash

set -xeo pipefail

if [ ! -x /bin/rpm ] || [ $(rpm --eval "%{centos_ver}") != "7" ]; then
    exit 0
fi

yum install -y \
    automake \
    clang \
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
