#include "asio_coro/dispatch.hpp"
#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio/io_context.hpp>

BOOST_AUTO_TEST_SUITE(test_dispatch)

BOOST_AUTO_TEST_CASE(test_dispatch) {
    auto running_within_strand_context_1 = false;
    auto running_within_strand_context_2 = false;
    boost::asio::io_context context;
    boost::asio::io_context::strand strand(context);
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        running_within_strand_context_1 = strand.running_in_this_thread();
        co_await asio_coro::dispatch(strand);
        running_within_strand_context_2 = strand.running_in_this_thread();
    });

    context.run();

    BOOST_REQUIRE(!running_within_strand_context_1);
    BOOST_REQUIRE(running_within_strand_context_2);
}

BOOST_AUTO_TEST_SUITE_END()
