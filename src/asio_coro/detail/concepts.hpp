#ifndef ASIO_CORO_EXTENSIONS_DETAIL_CONCEPTS_HPP
#define ASIO_CORO_EXTENSIONS_DETAIL_CONCEPTS_HPP

#include <boost/system/error_code.hpp>

#include <type_traits>

namespace asio_coro::detail {
template<class T> concept completion_condition = requires(T a) {
    {
    a(std::declval<const boost::system::error_code &>(),
            std::declval<std::size_t>())
    } -> std::convertible_to<std::size_t>;
};
} // namespace asio_coro::detail

#endif // ASIO_CORO_EXTENSIONS_DETAIL_CONCEPTS_HPP
