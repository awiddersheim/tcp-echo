# About

Just messing around with C, pthreads, SO_RESUSEPORT and CMake.

# Building

* Create and enter build directory: `mkdir build && cd build`
* Create build environment: `cmake ..`
* Build: `cmake --build .`

# Running

After building, run the server `src/tcp-echo`.

# Testing

Send the server some data:

```
$ nc localhost 8090
foo
foo
bar
bar
```
