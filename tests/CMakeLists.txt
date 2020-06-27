project(asio_coro_extensions_tests)

set(SOURCES
        tests_main.cpp
        test_spawn_coroutine.cpp
        test_async_wait.cpp
        test_async_socket_ops.cpp
        test_task.cpp
        test_dispatch.cpp
        test_post.cpp)

add_executable(asio_coro_extensions_tests ${SOURCES})
target_link_libraries(asio_coro_extensions_tests PUBLIC asio_coro_extensions Boost::unit_test_framework)
