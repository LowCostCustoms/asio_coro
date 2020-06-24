#ifndef ASIO_CORO_EXTENSIONS_ASYNC_WAIT_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_WAIT_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/system/error_code.hpp>
#include <boost/scope_exit.hpp>

#include <coroutine>

namespace asio_coro {
/**
 * Returns an awaitable that suspends the awaiting coroutine until the timer callback has been called.
 *
 * @param timer     Timer to await. Should be armed with expires_from_now or expires_at method call before calling
 *                  this method.
 */
template<class Timer>
auto async_wait(Timer &timer) {
    class awaitable {
    public:
        explicit awaitable(Timer &timer) : _timer(timer) {
            // Nothing to do.
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        boost::system::error_code await_resume() const noexcept {
            return _result;
        }

        void await_suspend(std::coroutine_handle<> continuation) {
            detail::coroutine_holder holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&holder) {
                holder.release();
            };

            _timer.async_wait([this, holder = std::move(holder)](const boost::system::error_code &error) mutable {
                _result = error;
                holder.release().resume();
            });
        }

    private:
        Timer &_timer;
        boost::system::error_code _result;
    };

    return awaitable(timer);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_WAIT_HPP
