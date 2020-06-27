#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>

#include <memory>


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

BOOST_AUTO_TEST_CASE(test_spawn_coroutine_frees_resources_when_coroutine_finished) {
    boost::asio::io_context context;
    const auto ptr = std::make_shared<int>(100);
    asio_coro::spawn_coroutine(context, [ptr]() -> asio_coro::task<void> {
        co_return;
    });

    BOOST_REQUIRE_EQUAL(ptr.use_count(), 2);

    context.run();
    BOOST_REQUIRE_EQUAL(ptr.use_count(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
