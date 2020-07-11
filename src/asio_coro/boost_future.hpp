#ifndef ASIO_CORO_EXTENSIONS_FUTURE_HPP
#define ASIO_CORO_EXTENSIONS_FUTURE_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/thread/future.hpp>
#include <boost/scope_exit.hpp>

#ifdef BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

namespace asio_coro {
/**
 * Returns an awaitable that suspends the awaiting coroutine until the future is resolved and then immediately runs
 * the awaiting coroutine.
 */
template<class ResultType>
auto async_wait_future(boost::future<ResultType> &&future) {
    class awaitable {
    public:
        explicit awaitable(boost::future<ResultType> &&future) noexcept
                : _future(std::move(future)) {
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        decltype(auto) await_resume() {
            return _then_future.get();
        }

        void await_suspend(detail::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            _future.then(boost::launch::sync, [this, holder = std::move(holder)](auto &&future) mutable {
                _then_future = std::move(future);
                holder.release().resume();
            });
        }

    private:
        boost::future<ResultType> _future;
        boost::future<ResultType> _then_future;
    };

    return awaitable(std::move(future));
}

/**
 * Returns an awaitable that suspends the awaiting coroutine until the future is resolved and then schedules the
 * coroutine execution within specified ASIO executor.
 */
template<class Executor, class ResultType>
auto async_wait_future(Executor &executor, boost::future<ResultType> &&future) {
    class awaitable {
    public:
        explicit awaitable(Executor &executor, boost::future<ResultType> &&future)
                : _executor(executor), _future(std::move(future)) {
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        decltype(auto) await_resume() {
            return _then_future.get();
        }

        void await_suspend(detail::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            _future.then(boost::launch::sync, [this, holder = std::move(holder)](auto &&future) mutable {
                _then_future = std::move(future);

                boost::asio::dispatch(_executor, [holder = std::move(holder)]() mutable {
                    holder.release().resume();
                });
            });
        }

    private:
        Executor &_executor;
        boost::future<ResultType> _future;
        boost::future<ResultType> _then_future;
    };

    return awaitable(executor, std::move(future));
}
} // namespace asio_coro

#endif // BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

#endif // ASIO_CORO_EXTENSIONS_FUTURE_HPP
