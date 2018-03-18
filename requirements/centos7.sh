#!/bin/sh

set -xe

if [ "$(grep -c 'CentOS Linux 7' /etc/os-release)" == "0" ]; then
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
