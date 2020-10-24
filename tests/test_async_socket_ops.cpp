#include "asio_coro/asio_coro.hpp"

#include "catch2/catch.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

TEST_CASE("async_accept/async_connect return an awaitable that resumes a coroutine upon the connection is "
          "accepted/established") {
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
    REQUIRE(!accept_error);

    accepted = true;
  });

  asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
    boost::asio::ip::tcp::socket socket(context);
    const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
    REQUIRE(!connect_error);

    connected = true;
  });

  context.run();

  REQUIRE(accepted);
  REQUIRE(connected);
}

TEST_CASE("async_read/async_write returns an awaitable that resumes a coroutine upon data is read/written") {
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
    REQUIRE(!accept_error);

    const auto [read_error, size] = co_await asio_coro::async_read(
        accepted_socket, boost::asio::mutable_buffer(received_data.data(), received_data.size()));
    REQUIRE(!read_error);
  });

  asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
    boost::asio::ip::tcp::socket socket(context);
    const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
    REQUIRE(!connect_error);

    const auto [write_error, size] = co_await asio_coro::async_write(socket, boost::asio::buffer(data_to_send));
    REQUIRE(!write_error);
  });

  context.run();

  REQUIRE(received_data == data_to_send);
}

TEST_CASE("async read/write can use completion conditions") {
  std::string data_to_send(1000000, 'r');
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
    REQUIRE(!accept_error);

    const auto read_data = received_data.data();
    const auto read_size = received_data.size();
    const auto [read_error, size] = co_await asio_coro::async_read(
        accepted_socket, boost::asio::mutable_buffer(read_data, read_size), boost::asio::transfer_exactly(read_size));
    REQUIRE(!read_error);
    REQUIRE(size == read_size);
  });

  asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
    boost::asio::ip::tcp::socket socket(context);
    const auto connect_error = co_await asio_coro::async_connect(socket, endpoint);
    REQUIRE(!connect_error);

    const auto write_data = data_to_send.data();
    const auto write_size = data_to_send.size();
    const auto [write_error, size] = co_await asio_coro::async_write(
        socket, boost::asio::buffer(write_data, write_size), boost::asio::transfer_exactly(write_size));
    REQUIRE(!write_error);
    REQUIRE(size == write_size);
  });

  context.run();

  REQUIRE(received_data == data_to_send);
}
