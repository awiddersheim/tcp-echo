# tcp-echo

[![CircleCI](https://circleci.com/gh/awiddersheim/tcp-echo.svg?style=svg)](https://circleci.com/gh/awiddersheim/tcp-echo)

This started as a project to mess around with [C][c_lang],
[pthreads][pthreads], [SO_REUSEPORT][reuseport] and [CMake][cmake]. That
is largely still the case but have since transitioned away from pthreads
to [libuv][libuv].  Can look through version control history to see
initial pthread work.

This has sort of evolved to become a nano version of [NGINX's][nginx]
[architecture][nginx-arch] with a master process controlling multiple
workers handling incoming connections which are [distributed by the
kernel][nginx-reuseport] using `SO_REUSEPORT`.

```
                  tcp-echo[mastr]
            K| --> \_ tcp-echo[wrk1]
      o/    E| --> \_ tcp-echo[wrk2]
User /| --> R| --> \_ tcp-echo[wrk3]
     / \    N| --> \_ tcp-echo[wrk4]
            E| --> \_ tcp-echo[wrk5]
            L| --> \_ tcp-echo[wrk6]
```

## Table of Contents

* [Quickstart](#quickstart)
* [Building](#building)
  * [Docker](#docker)
  * [Manual](#manual)
* [Developing](#developing)
* [Testing](#testing)
  * [Valgrind](#valgrind)
  * [Performance](#performance)
* [Configuring](#configuring)
* [Communication](#communication)
* [Contributing](#contributing)

## Quickstart

The easiest way to get things running is with [Docker][docker]. These
are the requirements necessary:

* [Docker Engine][docker-engine-install]
* [Docker Compose][docker-compose-install]

Once the requirements are installed simply run the following:

```
$ docker-compose up
```

Now send some data to the server:

```
$ docker-compose exec tcp-echo nc localhost 8090
foo
foo
bar
bar
```

## Building

### Docker

Building everything with Docker is easy.

```
$ docker build .
```

Several base images are supported. Can check the `requirements`
directory to get an idea of which can be used and to add more.

Also, choosing between [GCC][gcc] and [Clang][clang] is possible.

```
$ docker build \
    --build-arg BASE_IMAGE=ubuntu:16.04 \
    --build-arg CC=/usr/bin/clang \
    --build-arg CXX=/usr/bin/clang++ \
    .
```

### Manual

Building manually can be a bit more involved as several requirements are
necessary on most platforms. Depending on what platform you are working
on, check the `requirements` directory for one that matches. In there
are all of the items necessary to begin building.

If building on Mac OSX then [Xcode][xcode] is required. From there, use
[homebrew][homebrew] to get the rest of these items:

```
$ brew install \
    autoconf \
    automake \
    cmake \
    libtool \
    valgrind
```

Regardless of platform, once you have all of the necessary requirements
and the code cloned locally, the project can be built using theses
steps:

* Create and enter build directory

```
$ mkdir build && cd build
```

* Generate build files

```
$ cmake ..
```

* Build the binaries

```
$ make
```

## Developing

Development is again, best done in Docker by using a development version
of the container which contains everything necessary.

```
$ docker-compose -f docker-compose.yml -f docker-compose.dev.yml up -d
```

Once the development container is running the locally cloned code will
be mounted as a volume to `/tcp-echo` so you can hack using local
editors. From there jump into the development container and build as
necessary.

```
$ docker-compose exec tcp-echo /bin/bash
$ cmake /tcp-echo
$ make
$ ./tcp-echo-master
```

The listening port `8090` is exposed locally so can interact with the
server using local tools there as well.

## Testing

There are some simple tests that can be run to ensure basic
functionality and no memory leaks. A testing version of the container
needs to built before running the tests.

```
$ docker build \
    --target test \
    --tag tcp-echo-test \
    --build-arg BASE_IMAGE=centos:7 \
    --build-arg CMAKE_OPTS=-DCMAKE_BUILD_TYPE=Debug \
    .
```

Once the testing image has been built, run the test script.

**NOTE**: At this time there are some problems with testing against the
Alpine base image. Some issues mainly with [musl libc][musl-libc].

```
$ ./test.sh
```

All of the tests are performed each commit to `master` by
[CircleCI][circleci]. Once all tests pass in the [pipeline][pipeline], a
new image is published to [Docker Hub][dockerhub].

### Valgrind

Can invoke Valgrind to test for memory leaks and other memory access
violations.

```
$ valgrind \
    --error-exitcode=1 \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --trace-children=yes \
    ./tcp-echo-master
```

### Performance

Been using [tcpkali][tcpkali] as a way to test throughput and
performance.

```
$ tcpkali localhost:8090 \
    --duration 90 \
    --dump-one \
    --connections 500 \
    --connect-rate 300 \
    --channel-lifetime 1 \
    --message foo \
    --message-rate 2
```

## Configuring

There are some configuration options available at build time in
`src/config.h.cmake`. Plans are under way for exposing more of these as
run time options through environment variables or a configuration file.

## Communication

**TODO**

## Contributing

**TODO**

[circleci]: https://circleci.com/
[c_lang]: https://en.wikipedia.org/wiki/C_(programming_language)
[clang]: http://clang.llvm.org/
[cmake]: https://cmake.org/
[docker]: https://www.docker.com/
[dockerhub]: https://hub.docker.com/r/awiddersheim/tcp-echo/
[docker-compose-install]: https://docs.docker.com/compose/install/
[docker-engine-install]: https://docs.docker.com/engine/installation/
[gcc]: https://gcc.gnu.org/
[homebrew]: https://brew.sh/
[libuv]: https://github.com/libuv/libuv
[musl-libc]: https://www.musl-libc.org/
[nginx]: https://www.nginx.com/
[nginx-arch]: https://www.nginx.com/blog/inside-nginx-how-we-designed-for-performance-scale/
[nginx-reuseport]: https://www.nginx.com/blog/socket-sharding-nginx-release-1-9-1/
[pipeline]: https://circleci.com/gh/awiddersheim/workflows/tcp-echo/tree/master
[pthreads]: https://en.wikipedia.org/wiki/POSIX_Threads
[reuseport]: https://lwn.net/Articles/542629/
[tcpkali]: https://github.com/satori-com/tcpkali
[xcode]: https://developer.apple.com/xcode/
