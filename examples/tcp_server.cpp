#include "asio_coro/task.hpp"
#include "asio_coro/async_accept.hpp"
#include "asio_coro/async_read.hpp"
#include "asio_coro/async_write.hpp"
#include "asio_coro/async_wait.hpp"
#include "asio_coro/async_wait_signal.hpp"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <memory>
#include <chrono>
#include <array>

class server : public std::enable_shared_from_this<server> {
public:
    server(boost::asio::ip::tcp::socket socket) : _socket(std::move(socket)) {
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
                        boost::system::error_code error;
                        self->_socket.close(error);

                        if (error) {
                            std::cout << "failed to close the socket: " << error.message() << "\n";
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
                    std::cout << "failed to read data from the socket: " << read_error.message() << "\n";
                    break;
                }

                std::cout << "read " << read_size << " bytes from " << remote_endpoint << "\n";

                self->_last_active_time = std::chrono::steady_clock::now();

                const auto [write_error, write_size] = co_await asio_coro::async_write(self->_socket,
                        boost::asio::buffer(buffer.data(), read_size),
                        boost::asio::transfer_exactly(read_size));
                if (write_error) {
                    std::cout << "failed to write data to the socket: " << write_error.message() << "\n";
                    break;
                }

                std::cout << "wrote " << write_size << " bytes to " << remote_endpoint << "\n";
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

        std::cout << "listening for incoming connections on " << endpoint << "\n";

        while (true) {
            boost::asio::ip::tcp::socket socket(context);
            const auto error = co_await asio_coro::async_accept(acceptor, socket);
            if (error) {
                std::cout << "failed to accept an incoming connection: " << error.message() << "\n";
                break;
            }

            std::cout << "an incoming connection accepted from " << socket.remote_endpoint() << "\n";

            start_server_coroutine(context, std::move(socket));
        }
    });
}

void start_shutdown_awaiter_coroutine(boost::asio::io_context &context) {
    asio_coro::spawn_coroutine(context, [&]() mutable -> asio_coro::task<void> {
        boost::asio::signal_set sigset(context, SIGTERM, SIGINT);
        const auto [error, signal] = co_await asio_coro::async_wait_signal(sigset);

        if (error) {
            std::cout << "failed to wait for a signal: " << error.message() << "\n";
        } else {
            const auto signal_name = signal == SIGTERM ? "SIGTERM" : signal == SIGINT ? "SIGINT" : "unknown";
            std::cout << "signal received " << signal_name << "\n";
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