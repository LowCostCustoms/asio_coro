#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>

#include <memory>

BOOST_AUTO_TEST_SUITE(test_task)

BOOST_AUTO_TEST_CASE(test_default_constructor) {
    asio_coro::task<void> task;
    BOOST_REQUIRE(!task.valid());
}

BOOST_AUTO_TEST_CASE(test_move_constructor) {
    auto coroutine_data = std::make_shared<int>(100);
    auto coroutine = [](auto data) -> asio_coro::task<void> {
        const auto local_data = std::move(data);
        co_return;
    };

    auto task = coroutine(coroutine_data);
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 2);
    BOOST_REQUIRE(task.valid());

    asio_coro::task<void> other_task(std::move(task));
    BOOST_REQUIRE(other_task.valid());
    BOOST_REQUIRE(!task.valid());

    other_task.release().resume();
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 1);
}

BOOST_AUTO_TEST_CASE(test_move_assignment) {
    auto coroutine_data = std::make_shared<int>(100);
    auto coroutine = [](auto data) -> asio_coro::task<void> {
        const auto local_data = std::move(data);
        co_return;
    };

    auto task = coroutine(coroutine_data);
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 2);
    BOOST_REQUIRE(task.valid());

    asio_coro::task<void> other_task;
    BOOST_REQUIRE(!other_task.valid());

    other_task = std::move(task);
    BOOST_REQUIRE(other_task.valid());
    BOOST_REQUIRE(!task.valid());

    other_task.release().resume();
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 1);
}

BOOST_AUTO_TEST_CASE(test_clear) {
    auto coroutine_data = std::make_shared<int>(100);
    auto coroutine = [](auto data) -> asio_coro::task<void> {
        const auto local_data = std::move(data);
        co_return;
    };

    auto task = coroutine(coroutine_data);
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 2);

    task.clear();
    BOOST_REQUIRE(!task.valid());
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 1);
}

BOOST_AUTO_TEST_CASE(test_destructor) {
    auto coroutine_data = std::make_shared<int>(100);
    {
        auto coroutine = [](auto data) -> asio_coro::task<void> {
            const auto local_data = std::move(data);
            co_return;
        };

        auto task = coroutine(coroutine_data);
        BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 2);
    }
    BOOST_REQUIRE_EQUAL(coroutine_data.use_count(), 1);
}

BOOST_AUTO_TEST_CASE(test_co_await) {
    auto result = 0;
    auto coroutine = [&result]() mutable -> asio_coro::task<int> {
        auto coroutine = []() -> asio_coro::task<int> {
            co_return 100;
        };
        result = (co_await coroutine()) * 3;
    };

    coroutine().release().resume();

    BOOST_REQUIRE_EQUAL(result, 300);
}

BOOST_AUTO_TEST_SUITE_END()
