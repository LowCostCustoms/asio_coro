#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <memory>

TEST_CASE("task default ctor creates invalid task") {
  asio_coro::task<void> task;
  REQUIRE(!task.valid());
}

TEST_CASE("task move ctor transfers ownership over the associated coroutine") {
  auto coroutine_data = std::make_shared<int>(100);
  auto coroutine = [](auto data) -> asio_coro::task<void> {
    const auto local_data = std::move(data);
    co_return;
  };

  auto task = coroutine(coroutine_data);
  REQUIRE(coroutine_data.use_count() == 2);
  REQUIRE(task.valid());

  asio_coro::task<void> other_task(std::move(task));
  REQUIRE(other_task.valid());
  REQUIRE(!task.valid());

  other_task.release().resume();
  REQUIRE(coroutine_data.use_count() == 1);
}

TEST_CASE("task move assignment destroys the associated coroutine and transfers ownership from moved coroutine") {
  auto coroutine_data = std::make_shared<int>(100);
  auto coroutine = [](auto data) -> asio_coro::task<void> {
    const auto local_data = std::move(data);
    co_return;
  };

  auto task = coroutine(coroutine_data);
  REQUIRE(coroutine_data.use_count() == 2);
  REQUIRE(task.valid());

  asio_coro::task<void> other_task;
  REQUIRE(!other_task.valid());

  other_task = std::move(task);
  REQUIRE(other_task.valid());
  REQUIRE(!task.valid());

  other_task.release().resume();
  REQUIRE(coroutine_data.use_count() == 1);
}

TEST_CASE("task clear destroys the associated coroutine") {
  auto coroutine_data = std::make_shared<int>(100);
  auto coroutine = [](auto data) -> asio_coro::task<void> {
    const auto local_data = std::move(data);
    co_return;
  };

  auto task = coroutine(coroutine_data);
  REQUIRE(coroutine_data.use_count() == 2);

  task.clear();
  REQUIRE(!task.valid());
  REQUIRE(coroutine_data.use_count() == 1);
}

TEST_CASE("task destructor destroys the associated coroutine") {
  auto coroutine_data = std::make_shared<int>(100);
  {
    auto coroutine = [](auto data) -> asio_coro::task<void> {
      const auto local_data = std::move(data);
      co_return;
    };

    auto task = coroutine(coroutine_data);
    REQUIRE(coroutine_data.use_count() == 2);
  }
  REQUIRE(coroutine_data.use_count() == 1);
}

TEST_CASE("task co_await operator returns an awaitable that resumes the associated coroutine") {
  auto result = 0;
  auto coroutine = [&result]() mutable -> asio_coro::task<int> {
    auto coroutine = []() -> asio_coro::task<int> { co_return 100; };
    result = (co_await coroutine()) * 3;
    co_return result;
  };

  coroutine().release().resume();

  REQUIRE(result == 300);
}
