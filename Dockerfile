ARG BASE_IMAGE=centos:7

# Development
FROM ${BASE_IMAGE} as dev

MAINTAINER Andrew Widdersheim <amwiddersheim@gmail.com>

COPY requirements /requirements

# NOTE(awiddersheim): A simple run-parts alternative.
RUN cd requirements \
    && for i in $(ls *.sh); do \
        test -f ${i} && test -x ${i} && echo "Running (${i})" && ./${i} \
    ; done

WORKDIR /build

STOPSIGNAL SIGKILL

CMD ["sleep", "infinity"]


# Build
FROM dev as build

ARG CC
ARG CXX
ARG CI

COPY . /tcp-echo

WORKDIR /build

RUN cmake /tcp-echo
RUN make -j4


# Test
FROM build as test

RUN useradd tcp-echo

USER tcp-echo


# Production
FROM ${BASE_IMAGE} as prod

RUN useradd tcp-echo

COPY --from=build /build/tcp-echo-master /build/tcp-echo-worker /tcp-echo/

WORKDIR /tcp-echo

EXPOSE 8090

USER tcp-echo
CMD ["./tcp-echo-master"]
