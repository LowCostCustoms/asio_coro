#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

TEST_CASE("async_wait returns an awaitable that resumes the coroutine upon the associated timer is triggered") {
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
  REQUIRE(!wait_error);
  REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(wait_expired_at - wait_started_at) >=
          std::chrono::milliseconds(100));
}
