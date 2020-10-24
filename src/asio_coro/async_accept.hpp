#ifndef ASIO_CORO_EXTENSIONS_ASYNC_ACCEPT_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_ACCEPT_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

namespace asio_coro {
/// Returns an awaitable that suspends the awaiting coroutine upon the either a new incoming connection is accepted or
/// an error is occurred during the connection establishment.
///
/// The awaitable returns a value of type boost::system::error_code that contains the operation result.
///
/// \param acceptor     A connection acceptor socket.
/// \param socket       An accepted socket.
template <class Acceptor, class Socket> auto async_accept(Acceptor &acceptor, Socket &socket) {
  class awaitable {
  public:
    explicit awaitable(Acceptor &acceptor, Socket &socket) noexcept : _acceptor(acceptor), _socket(socket) {}

    bool await_ready() const noexcept { return false; }

    boost::system::error_code await_resume() const noexcept { return _result; }

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      _acceptor.async_accept(_socket,
                             [this, holder = std::move(holder)](const boost::system::error_code &error) mutable {
                               _result = error;
                               holder.release().resume();
                             });
    }

  private:
    Acceptor &_acceptor;
    Socket &_socket;
    boost::system::error_code _result;
  };

  return awaitable(acceptor, socket);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_ACCEPT_HPP
