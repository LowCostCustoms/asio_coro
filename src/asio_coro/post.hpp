#ifndef ASIO_CORO_EXTENSIONS_POST_HPP
#define ASIO_CORO_EXTENSIONS_POST_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>

namespace asio_coro {
/// Returns an awaitable that suspend the awaiting coroutine and resumes it within the specified executor context using
/// boost::asio::post method.
///
/// \param executor   An executor to resume the awaiting coroutine on.
template <class Executor> auto post(Executor &executor) {
  class awaitable {
  public:
    explicit awaitable(Executor &executor) : _executor(executor) {}

    constexpr bool await_ready() const noexcept { return false; }

    constexpr void await_resume() const noexcept {}

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      boost::asio::post(_executor, [holder = std::move(holder)]() mutable { holder.release().resume(); });
    }

  private:
    Executor &_executor;
  };

  return awaitable(executor);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_POST_HPP
