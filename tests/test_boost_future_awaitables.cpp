#include "asio_coro/asio_coro.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>


BOOST_AUTO_TEST_SUITE(test_boost_future_awaitables)

BOOST_AUTO_TEST_CASE(test_await_boost_future) {
    boost::promise<int> promise;
    boost::promise<int> coroutine_promise;
    auto coroutine_future = coroutine_promise.get_future();
    boost::asio::io_context io_context;
    asio_coro::spawn_coroutine(io_context,
            [future = promise.get_future(), promise = std::move(coroutine_promise)]() mutable -> asio_coro::task<void> {
                const auto result = co_await asio_coro::async_wait_future(std::move(future));
                promise.set_value(result);
            });

    promise.set_value(100);

    io_context.run();

    BOOST_REQUIRE_EQUAL(coroutine_future.get(), 100);
}

BOOST_AUTO_TEST_CASE(test_await_boost_future_with_custom_executor) {
    boost::asio::io_context io_context;
    boost::asio::io_context::strand strand(io_context);
    boost::promise<int> promise;
    auto future = promise.get_future();
    asio_coro::spawn_coroutine(io_context, [promise = std::move(promise), &strand]() mutable -> asio_coro::task<void> {
        boost::promise<int> internal_promise;
        internal_promise.set_value(100);

        const auto result = co_await asio_coro::async_wait_future(strand, internal_promise.get_future());
        if (strand.running_in_this_thread()) {
            promise.set_value(result);
        } else {
            promise.set_exception(std::runtime_error("promise wasn't resolved within strand context"));
        }
    });

    io_context.run();

    BOOST_REQUIRE_EQUAL(future.get(), 100);
}

BOOST_AUTO_TEST_SUITE_END()
