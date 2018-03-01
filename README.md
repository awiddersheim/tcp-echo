# About

[![CircleCI](https://circleci.com/gh/awiddersheim/tcp-echo.svg?style=svg)](https://circleci.com/gh/awiddersheim/tcp-echo)

Started as a project to mess around with [C][c_lang],
[pthreads][pthreads], [SO_REUSEPORT][reuseport] and [CMake][cmake]. That
is largely still the case but have since transitioned away from pthreads
to [libuv][libuv].  Can look through version control history to see
initial pthread work.

This has sort of evolved to become a nano version of [NGINX's][nginx]
[architecture][nginx-arch] with a master processes controlling multiple
workers handling incoming connections which are [distributed by the
kernel][nginx-reuseport] using `SO_REUSEPORT`.

# Building

* Create and enter build directory: `mkdir build && cd build`
* Create build environment: `cmake ..`
* Build: `cmake --build .`

# Running

After building, run the server `./tcp-echo-masater`.

# Testing

Send the server some data:

```
$ nc localhost 8090
foo
foo
bar
bar
```

[c_lang]: https://en.wikipedia.org/wiki/C_(programming_language)
[cmake]: https://cmake.org/
[libuv]: https://github.com/libuv/libuv
[nginx]: https://www.nginx.com/
[nginx-arch]: https://www.nginx.com/blog/inside-nginx-how-we-designed-for-performance-scale/
[nginx-reuseport]: https://www.nginx.com/blog/socket-sharding-nginx-release-1-9-1/
[pthreads]: https://en.wikipedia.org/wiki/POSIX_Threads
[reuseport]: https://lwn.net/Articles/542629/
