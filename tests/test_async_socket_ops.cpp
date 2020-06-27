#include "asio_coro/async_read.hpp"
#include "asio_coro/async_write.hpp"
#include "asio_coro/async_accept.hpp"
#include "asio_coro/async_connect.hpp"
#include "asio_coro/async_wait.hpp"
#include "asio_coro/task.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

BOOST_AUTO_TEST_SUITE(test_async_socket_ops)

BOOST_AUTO_TEST_CASE(test_async_accept_connect) {
    auto accepted = false;
    auto connected = false;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(0u), 64400);
    boost::asio::io_context context;
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::acceptor acceptor(context);
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();

        boost::asio::ip::tcp::socket accepted_socket(context);
        const auto accept_error = co_await asio_coro::async_accept(acceptor, accepted_socket);
        BOOST_REQUIRE(!accept_error);

        accepted = true;
    });

    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::socket socket(context);
        const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
        BOOST_REQUIRE(!connect_error);

        connected = true;
    });

    context.run();

    BOOST_REQUIRE(accepted);
    BOOST_REQUIRE(connected);
}

BOOST_AUTO_TEST_CASE(test_async_read_write) {
    std::string data_to_send = "data data data";
    std::string received_data(data_to_send.length(), '\0');

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(0u), 64401);
    boost::asio::io_context context;
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::acceptor acceptor(context);
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();

        boost::asio::ip::tcp::socket accepted_socket(context);
        const auto accept_error = co_await asio_coro::async_accept(acceptor, accepted_socket);
        BOOST_REQUIRE(!accept_error);

        const auto [read_error, size] = co_await asio_coro::async_read(accepted_socket,
                boost::asio::mutable_buffer(received_data.data(), received_data.size()));
        BOOST_REQUIRE(!read_error);
    });

    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::socket socket(context);
        const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
        BOOST_REQUIRE(!connect_error);

        const auto [write_error, size] = co_await asio_coro::async_write(socket, boost::asio::buffer(data_to_send));
        BOOST_REQUIRE(!write_error);
    });

    context.run();

    BOOST_REQUIRE_EQUAL(received_data, data_to_send);
}

BOOST_AUTO_TEST_CASE(test_async_read_write_with_completion_condition) {
    std::string data_to_send(1000000,  'r');
    std::string received_data(data_to_send.length(), '\0');

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(0u), 64401);
    boost::asio::io_context context;
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::acceptor acceptor(context);
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();

        boost::asio::ip::tcp::socket accepted_socket(context);
        const auto accept_error = co_await asio_coro::async_accept(acceptor, accepted_socket);
        BOOST_REQUIRE(!accept_error);

        const auto read_data = received_data.data();
        const auto read_size = received_data.size();
        const auto [read_error, size] = co_await asio_coro::async_read(accepted_socket,
            boost::asio::mutable_buffer(read_data, read_size),
            boost::asio::transfer_exactly(read_size));
        BOOST_REQUIRE(!read_error);
        BOOST_REQUIRE_EQUAL(size, read_size);
    });

    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::socket socket(context);
        const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
        BOOST_REQUIRE(!connect_error);

        const auto write_data = data_to_send.data();
        const auto write_size = data_to_send.size();
        const auto [write_error, size] = co_await asio_coro::async_write(socket,
                boost::asio::buffer(write_data, write_size),
                boost::asio::transfer_exactly(write_size));
        BOOST_REQUIRE(!write_error);
        BOOST_REQUIRE_EQUAL(size, write_size);
    });

    context.run();

    BOOST_REQUIRE_EQUAL(received_data, data_to_send);
}

BOOST_AUTO_TEST_SUITE_END()
