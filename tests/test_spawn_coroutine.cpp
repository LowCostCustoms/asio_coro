#include "asio_coro/task.hpp"

#include "catch2/catch.hpp"

#include <boost/asio/io_context.hpp>

#include <memory>

TEST_CASE("spawn_coroutine runs coroutine within specified io_context") {
  boost::asio::io_context context;
  auto sum = 0;
  for (auto i = 0; i != 10; ++i) {
    asio_coro::spawn_coroutine(context, [&sum, i]() -> asio_coro::task<void> {
      sum += i;
      co_return;
    });
  }

  context.run();
  REQUIRE(sum == 45);
}

TEST_CASE("spawn_coroutine destroys coroutine context upon it's finished") {
  boost::asio::io_context context;
  const auto ptr = std::make_shared<int>(100);
  asio_coro::spawn_coroutine(context, [ptr]() -> asio_coro::task<void> { co_return; });

  REQUIRE(ptr.use_count() == 2);

  context.run();
  REQUIRE(ptr.use_count() == 1);
}
