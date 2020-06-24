cmake_minimum_required(VERSION 3.17)
project(asio_coro_extensions)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED system thread)

add_executable(asio_coro_extensions library.cpp library.h)
target_link_libraries(asio_coro_extensions ${Boost_LIBRARIES})
target_compile_options(asio_coro_extensions PUBLIC -fcoroutines)
target_include_directories(asio_coro_extensions PUBLIC src)
target_compile_definitions(asio_coro_extensions PUBLIC BOOST_ASIO_DISABLE_CONCEPTS)
