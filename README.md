# About

[![CircleCI](https://circleci.com/gh/awiddersheim/tcp-echo.svg?style=svg)](https://circleci.com/gh/awiddersheim/tcp-echo)

Just messing around with C, libuv, SO_RESUSEPORT and CMake.

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
