#ifndef ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/system/error_code.hpp>
#include <boost/scope_exit.hpp>

#include <utility>

namespace asio_coro {
using async_read_result = std::pair<boost::system::error_code, std::size_t>;

template<class Stream, class MutableBuffer>
auto async_read(Stream &stream, const MutableBuffer &buffer) {
    class awaitable {
    public:
        awaitable(Stream &stream, const MutableBuffer &buffer)
                : _stream(stream), _buffer(buffer) {
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        async_read_result await_resume() const noexcept {
            return _result;
        }

        void await_suspend(std::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            _stream.async_read_some(_buffer,
                    [this, holder = std::move(holder)](const boost::system::error_code &error,
                            std::size_t size) mutable {
                        _result = std::make_pair(error, size);
                        holder.release().resume();
                    });
        }

    private:
        Stream &_stream;
        MutableBuffer _buffer;
        async_read_result _result;
    };

    return awaitable(stream, buffer);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP
