cmake_minimum_required(VERSION 3.5)
project(asio_coro_extensions)

option(WITH_TESTS "Build tests" ON)
option(WITH_EXAMPLES "Build examples" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)

set(REQUIRED_BOOST_LIBRARIES system thread)
if (${WITH_TESTS})
    list(APPEND REQUIRED_BOOST_LIBRARIES unit_test_framework)
endif()

find_package(Boost REQUIRED ${REQUIRED_BOOST_LIBRARIES})

set(HEADERS
        src/asio_coro/asio_coro.hpp
        src/asio_coro/async_accept.hpp
        src/asio_coro/async_connect.hpp
        src/asio_coro/async_mutex.hpp
        src/asio_coro/async_read.hpp
        src/asio_coro/async_wait_signal.hpp
        src/asio_coro/async_wait.hpp
        src/asio_coro/async_write.hpp
        src/asio_coro/boost_future.hpp
        src/asio_coro/dispatch.hpp
        src/asio_coro/post.hpp
        src/asio_coro/task.hpp
        src/asio_coro/detail/coroutine_holder.hpp
        src/asio_coro/detail/coroutine.hpp
        src/asio_coro/detail/task.hpp)

add_library(asio_coro_extensions ${HEADERS})
target_link_libraries(asio_coro_extensions PUBLIC Boost::thread Boost::system)
set_target_properties(asio_coro_extensions PROPERTIES LINKER_LANGUAGE CXX)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(asio_coro_extensions PUBLIC -fcoroutines)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(asio_coro_extensions PUBLIC -fcoroutines-ts)
endif()
target_include_directories(asio_coro_extensions PUBLIC src)
target_compile_definitions(asio_coro_extensions PUBLIC BOOST_ASIO_DISABLE_CONCEPTS)

if(${WITH_TESTS})
    add_subdirectory(tests)
endif()

if(${WITH_EXAMPLES})
    find_package(FMT REQUIRED)
    add_subdirectory(examples)
endif()
