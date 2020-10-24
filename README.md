# ASIO coroutine extensions

This library provides extensions for Boost.ASIO library to work with C++20
coroutines conveniently. It's rather a research library, so production-readiness
is not guaranteed.

## Requirements

This library requires Boost.ASIO and Boost.THREAD libraries itself.
To run tests and examples you also need to have fmtlib library installed.

## Building

Building of this project requires you to have CMake, ex:

```shell script
mkdir build && cd build

cmake -G"Unix Makefiles" ../ \
  -DCMAKE_BUILD_TYPE=Debug   \
  -DWITH_TESTS=1             \
  -DWITH_EXAMPLES=1

make
```

## Examples

To see the examples, look at the `/examples` folder in the project root.

## Usage

This library is header-only so its usage is pretty simple:

```c++
#include <asio_coro/asio_coro.hpp>

#include <boost/asio/io_context.hpp>

int main() {
    boost::asio::io_context io_context;
    asio_coro::spawn_coroutine(io_context, []() -> asio_coro::task<void> {
        // whatever
        co_return;
    });
    
    io_context.run();
}
```

There are also extensions for awaiting `boost::future` results, but
unfortunately Boost.Thread doesn't define `BOOST_THREAD_PROVIDES_FUTURE`
and `BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION` by default, so you need
to specify them manually in your project (see tests for cmake target
configuration details).
