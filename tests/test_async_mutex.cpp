#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

namespace {
template <typename Predicate>
asio_coro::void_task wait_for_cond(boost::asio::io_context &context, asio_coro::async_mutex &mutex,
                                   Predicate &&predicate) {
  while (true) {
    {
      const auto lock_guard = co_await mutex.async_lock_scoped();
      if (predicate()) {
        break;
      }
    }
    co_await asio_coro::post(context);
  }
}

template <typename Function>
asio_coro::void_task lock_and_run(boost::asio::io_context &context, asio_coro::async_mutex &mutex,
                                  Function &&function) {
  {
    const auto lock_guard = co_await mutex.async_lock_scoped();
    function();
  }
  co_await asio_coro::post(context);
}
} // namespace

TEST_CASE("mutex::async_lock* suspends a coroutine upon the held lock is released, mutex::unlock resumes the awaiting "
          "coroutine if any") {
  constexpr auto coroutine_0_id = "coroutine 1";
  constexpr auto coroutine_1_id = "coroutine 2";
  std::vector<std::string_view> expected_mod_order{
      coroutine_0_id,
      coroutine_1_id,
      coroutine_1_id,
      coroutine_0_id,
  };

  boost::asio::io_context io_context;
  asio_coro::async_mutex mutex;

  std::vector<std::string_view> mod_order;
  auto counter = 0u;

  // Spawn first coroutine.
  asio_coro::spawn_coroutine(io_context, [&]() mutable -> asio_coro::void_task {
    co_await lock_and_run(io_context, mutex, [&]() {
      counter = 1;
      mod_order.emplace_back(coroutine_0_id);
    });

    co_await wait_for_cond(io_context, mutex, [&]() { return counter == 3; });

    co_await lock_and_run(io_context, mutex, [&]() {
      counter = 4;
      mod_order.emplace_back(coroutine_0_id);
    });
  });

  // Spawn second coroutine.
  asio_coro::spawn_coroutine(io_context, [&]() mutable -> asio_coro::void_task {
    co_await wait_for_cond(io_context, mutex, [&]() { return counter == 1; });

    for (auto i = 2u; i != 4u; ++i) {
      co_await lock_and_run(io_context, mutex, [&]() {
        counter = i;
        mod_order.emplace_back(coroutine_1_id);
      });
    }

    co_await wait_for_cond(io_context, mutex, [&]() { return counter == 4; });
  });

  // Run coroutines until they finished.
  io_context.run();

  // Check whether they were running in some particular order.
  REQUIRE(mod_order == expected_mod_order);
}
