#!/bin/bash

set -xeo pipefail

if [ "$(grep -c 'xenial' /etc/os-release)" == "0" ]; then
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
