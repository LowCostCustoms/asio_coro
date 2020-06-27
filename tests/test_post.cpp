#include "asio_coro/post.hpp"
#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>

BOOST_AUTO_TEST_SUITE(test_post)

BOOST_AUTO_TEST_CASE(test_post) {
    auto finished = false;

    boost::asio::io_context context_1;
    boost::asio::io_context context_2;
    asio_coro::spawn_coroutine(context_1, [&]() mutable -> asio_coro::task<void> {
        co_await asio_coro::post(context_2);
        finished = true;
    });

    context_1.run();
    context_2.run();

    BOOST_REQUIRE(finished);
}

BOOST_AUTO_TEST_SUITE_END()
