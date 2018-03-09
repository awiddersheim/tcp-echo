FROM centos:7

MAINTAINER Andrew Widdersheim <amwiddersheim@gmail.com>

RUN yum install -y \
      automake \
      cmake \
      gcc \
      gcc-c++ \
      git \
      libtool \
      make \
    && yum clean all \
    && useradd tcp-echo

COPY . /tcp-echo

WORKDIR /tcp-echo/build

RUN cmake .. \
    && make -j4

EXPOSE 8090

USER tcp-echo
ENTRYPOINT ["./tcp-echo-master"]
