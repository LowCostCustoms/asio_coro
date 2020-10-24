#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <boost/asio/io_context.hpp>

TEST_CASE("post returns an awaitable that resumes coroutine within specified io_context") {
  auto finished = false;

  boost::asio::io_context context_1;
  boost::asio::io_context context_2;
  asio_coro::spawn_coroutine(context_1, [&]() mutable -> asio_coro::task<void> {
    co_await asio_coro::post(context_2);
    finished = true;
  });

  context_1.run();
  context_2.run();

  REQUIRE(finished);
}
