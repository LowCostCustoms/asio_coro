#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <boost/asio/io_context.hpp>

TEST_CASE("dispatch returns an awaitable that resumes coroutine within specified io_context") {
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

  REQUIRE(!running_within_strand_context_1);
  REQUIRE(running_within_strand_context_2);
}
