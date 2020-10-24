#ifndef ASIO_CORO_EXTENSIONS_ASYNC_WAIT_SIGNAL_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_WAIT_SIGNAL_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

#include <utility>

namespace asio_coro {
using async_wait_signal_result = std::pair<boost::system::error_code, int>;

/// Returns an awaitable that suspends the awaiting coroutine upon either one of the signals in the specified signal set
/// is emitted or an error is occurred during awaiting these signals.
///
/// The awaitable returns an instance of async_wait_signal_result, where the first item contains the result of the
/// operation and the second - the identifier of the signal that lead to coroutine resumption.
///
/// \param signal_set   A set of signals to await.
template <class SignalSet> auto async_wait_signal(SignalSet &signal_set) {
  class awaitable {
  public:
    explicit awaitable(SignalSet &signal_set) noexcept : _signal_set(signal_set) {}

    bool await_ready() const noexcept { return false; }

    async_wait_signal_result await_resume() const noexcept { return _result; }

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      _signal_set.async_wait(
          [this, holder = std::move(holder)](const boost::system::error_code &error, int signal) mutable {
            _result = std::make_pair(error, signal);
            holder.release().resume();
          });
    }

  private:
    SignalSet &_signal_set;
    async_wait_signal_result _result;
  };

  return awaitable(signal_set);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_WAIT_SIGNAL_HPP
