#include "asio_coro/task.hpp"
#include "asio_coro/async_accept.hpp"
#include "asio_coro/async_read.hpp"
#include "asio_coro/async_write.hpp"
#include "asio_coro/async_wait.hpp"
#include "asio_coro/async_wait_signal.hpp"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <fmt/format.h>

#include <iostream>
#include <memory>
#include <chrono>
#include <array>


template <>
struct fmt::formatter<boost::asio::ip::tcp::endpoint> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template<class FormatContext>
    auto format(const boost::asio::ip::tcp::endpoint &endpoint, FormatContext &ctx) {
        return format_to(ctx.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
    }
};


class server : public std::enable_shared_from_this<server> {
public:
    explicit server(boost::asio::ip::tcp::socket socket) : _socket(std::move(socket)) {
    }

    void run(boost::asio::io_context &context) {
        _last_active_time = std::chrono::steady_clock::now();

        auto weak_self = weak_from_this();
        asio_coro::spawn_coroutine(context, [&context, weak_self = std::move(weak_self)]() -> asio_coro::task<void> {
            boost::asio::steady_timer timer(context);

            while (true) {
                timer.expires_from_now(std::chrono::seconds(1));
                const auto error = co_await asio_coro::async_wait(timer);
                if (error) {
                    break;
                }

                if (const auto self = weak_self.lock(); self) {
                    if (std::chrono::steady_clock::now() - self->_last_active_time > std::chrono::seconds(5)) {
                        const auto remote_endpoint = self->_socket.remote_endpoint();
                        fmt::print("closing connection {} die to inactivity timeout\n", remote_endpoint);

                        boost::system::error_code error;
                        self->_socket.close(error);

                        if (error) {
                            fmt::print("failed to close connection {}: {}\n", remote_endpoint, error.message());
                            break;
                        }
                    }
                } else {
                    break;
                }
            }
        });

        auto self = shared_from_this();
        asio_coro::spawn_coroutine(context, [self = std::move(self)]() -> asio_coro::task<void> {
            const auto remote_endpoint = self->_socket.remote_endpoint();
            while (true) {
                std::array<char, 1024> buffer;
                const auto [read_error, read_size] = co_await asio_coro::async_read(self->_socket,
                        boost::asio::mutable_buffer(buffer.data(), buffer.size()));
                if (read_error) {
                    fmt::print("failed to read data from {}: {}\n", remote_endpoint, read_error.message());
                    break;
                }

                fmt::print("read {} bytes from {}\n", read_size, remote_endpoint);

                self->_last_active_time = std::chrono::steady_clock::now();

                const auto [write_error, write_size] = co_await asio_coro::async_write(self->_socket,
                        boost::asio::buffer(buffer.data(), read_size),
                        boost::asio::transfer_exactly(read_size));
                if (write_error) {
                    fmt::print("failed to write data to {}: {}\n", remote_endpoint, write_error.message());
                    break;
                }

                fmt::print("wrote {} bytes to {}\n", write_size, remote_endpoint);
            }
        });
    }

private:
    boost::asio::ip::tcp::socket _socket;
    std::chrono::steady_clock::time_point _last_active_time;
};

void start_server_coroutine(boost::asio::io_context &context, boost::asio::ip::tcp::socket socket) {
    const auto server_instance = std::make_shared<server>(std::move(socket));
    server_instance->run(context);
}

void start_accept_connections_coroutine(boost::asio::io_context &context) {
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(0u), 65400);
        boost::asio::ip::tcp::acceptor acceptor(context);
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();

        fmt::print("listening for incoming connections on {}\n", endpoint);

        while (true) {
            boost::asio::ip::tcp::socket socket(context);
            const auto error = co_await asio_coro::async_accept(acceptor, socket);
            if (error) {
                fmt::print("failed to accept an incoming connection: {}\n", error.message());
                break;
            }

            fmt::print("an incoming connection accepted from {}\n", socket.remote_endpoint());

            start_server_coroutine(context, std::move(socket));
        }
    });
}

void start_shutdown_awaiter_coroutine(boost::asio::io_context &context) {
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::signal_set sigset(context, SIGTERM, SIGINT);
        const auto [error, signal] = co_await asio_coro::async_wait_signal(sigset);

        if (error) {
            fmt::print("failed to wait for a signal: {}\n", error.message());
        } else {
            fmt::print("signal {} received, stopping the io context\n", signal);
        }

        context.stop();
    });
}

int main() {
    boost::asio::io_context context;
    start_accept_connections_coroutine(context);
    start_shutdown_awaiter_coroutine(context);

    context.run();

    return 0;
}