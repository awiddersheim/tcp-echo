# Development
FROM centos:7 as dev

MAINTAINER Andrew Widdersheim <amwiddersheim@gmail.com>

RUN yum install -y \
      automake \
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

WORKDIR /build

STOPSIGNAL SIGKILL

CMD ["sleep", "infinity"]


# Development
FROM dev as build

COPY . /tcp-echo

WORKDIR /build

RUN cmake /tcp-echo
RUN make -j4


# Production
FROM centos:7 as prod

RUN useradd tcp-echo

COPY --from=build /build/tcp-echo-master /build/tcp-echo-worker /tcp-echo/

WORKDIR /tcp-echo

EXPOSE 8090

USER tcp-echo
CMD ["./tcp-echo-master"]
