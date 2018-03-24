#!/bin/sh

set -xe

. /etc/os-release

if [ "${ID}" != "ubuntu" ] || [ "${VERSION_ID}" != "16.04" ]; then
    exit 0
fi

apt-get update
apt-get -y install \
    automake \
    clang \
    cmake \
    gcc \
    g++ \
    git \
    libtool \
    make \
    netcat \
    telnet \
    valgrind \
    vim
