#ifndef ASIO_CORO_EXTENSIONS_DISPATCH_HPP
#define ASIO_CORO_EXTENSIONS_DISPATCH_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>

namespace asio_coro {
/**
 * Submits the awaiting coroutine for execution within the specified executor. See boost::asio::dispatch for details.
 */
template<class Executor>
auto dispatch(Executor &executor) {
    class awaitable {
    public:
        explicit awaitable(Executor &executor)
                : _executor(executor) {
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        constexpr void await_resume() const noexcept {
        }

        void await_suspend(std::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            boost::asio::dispatch(_executor, [holder = std::move(holder)]() mutable {
                holder.release().resume();
            });
        }

    private:
        Executor &_executor;
    };

    return awaitable(executor);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_DISPATCH_HPP
