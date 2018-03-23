ARG BASE_IMAGE=alpine:3.7

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

ARG ANALYZER
ARG CC
ARG CXX
ARG CI
ARG CMAKE_OPTS
ARG MAKE_OPTS=-j4

COPY . /tcp-echo

WORKDIR /build

RUN cmake ${CMAKE_OPTS} /tcp-echo
RUN ${ANALYZER} make ${MAKE_OPTS}


# Test
FROM build as test

RUN adduser -D tcp-echo > /dev/null 2>&1 || useradd tcp-echo

USER tcp-echo


# Production
FROM ${BASE_IMAGE} as prod

RUN adduser -D tcp-echo > /dev/null 2>&1 || useradd tcp-echo

COPY --from=build /build/tcp-echo-master /build/tcp-echo-worker /tcp-echo/

WORKDIR /tcp-echo

EXPOSE 8090

USER tcp-echo
CMD ["./tcp-echo-master"]
