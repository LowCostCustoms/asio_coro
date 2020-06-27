#include "asio_coro/detail/coroutine_holder.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/scope_exit.hpp>


class test_task {
public:
    class promise_type {
    public:
        constexpr auto initial_suspend() noexcept {
            return std::suspend_always{};
        }

        constexpr auto final_suspend() noexcept {
            return std::suspend_never{};
        }

        constexpr void return_void() noexcept {
        }

        void unhandled_exception() {
            std::terminate();
        }

        constexpr auto get_return_object() noexcept {
            return test_task{};
        }
    };
};


BOOST_AUTO_TEST_SUITE(coroutine_holder_tests)

BOOST_AUTO_TEST_CASE(coroutine_holder_move_constructor_test) {
    BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_CASE(coroutine_holder_move_assignment_test) {
    BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_CASE(coroutine_holder_release_test) {
    BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_CASE(coroutine_holder_clear_test) {
    BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_SUITE_END()
