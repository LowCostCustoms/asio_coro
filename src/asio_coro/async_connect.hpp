#ifndef ASIO_CORO_EXTENSIONS_ASYNC_CONNECT_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_CONNECT_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

namespace asio_coro {
/// Returns an awaitable that schedules an async socket connect routine and suspends the awaiting coroutine upon either
/// the connection is established or an error is occurred during the connection establishment.
///
/// The awaitable returns a value of type boost::system::error_code that contains the operation result.
///
/// \param socket   A socket that connects to the remote peer.
/// \param endpint  The remote peer endpoint.
template <class Socket, class Endpoint> auto async_connect(Socket &socket, const Endpoint &endpoint) {
  class awaitable {
  public:
    explicit awaitable(Socket &socket, const Endpoint &endpoint) : _socket(socket), _endpoint(endpoint) {}

    constexpr bool await_ready() const noexcept { return false; }

    boost::system::error_code await_resume() const noexcept { return _result; }

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      _socket.async_connect(_endpoint,
                            [this, holder = std::move(holder)](const boost::system::error_code &error) mutable {
                              _result = error;
                              holder.release().resume();
                            });
    }

  private:
    Socket &_socket;
    Endpoint _endpoint;
    boost::system::error_code _result;
  };

  return awaitable(socket, endpoint);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_CONNECT_HPP
