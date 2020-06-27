#include "asio_coro/async_wait.hpp"
#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

BOOST_AUTO_TEST_SUITE(test_async_wait)

BOOST_AUTO_TEST_CASE(test_async_wait) {
    boost::system::error_code wait_error;
    std::chrono::steady_clock::time_point wait_started_at;
    std::chrono::steady_clock::time_point wait_expired_at;
    boost::asio::io_context context;
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::steady_timer timer(context);

        wait_started_at = std::chrono::steady_clock::now();
        timer.expires_at(wait_started_at + std::chrono::milliseconds(100));
        wait_error = co_await asio_coro::async_wait(timer);

        wait_expired_at = std::chrono::steady_clock::now();
    });

    context.run();
    BOOST_REQUIRE(!wait_error);
    BOOST_REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(wait_expired_at - wait_started_at)
            >= std::chrono::milliseconds(100));
}

BOOST_AUTO_TEST_SUITE_END()
