#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>


BOOST_AUTO_TEST_SUITE(test_coroutine_spawn)

BOOST_AUTO_TEST_CASE(test_spawn) {
    boost::asio::io_context context;
    auto sum = 0;
    for (auto i = 0; i != 10; ++i) {
        asio_coro::spawn_coroutine(context, [&sum, i]() -> asio_coro::task<void> {
            sum += i;
            co_return;
        });
    }

    context.run();

    BOOST_REQUIRE_EQUAL(sum, 45);
}

BOOST_AUTO_TEST_CASE(test_spawn_doesnt_destroy_lambda_on_exit) {
    BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_SUITE_END()
