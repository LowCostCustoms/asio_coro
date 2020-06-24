#include "library.h"

#include "asio_coro/task.hpp"
#include "asio_coro/async_wait.hpp"
#include "asio_coro/mutex.hpp"
#include "asio_coro/async_accept.hpp"
#include "asio_coro/async_read.hpp"
#include "asio_coro/async_write.hpp"
#include "asio_coro/async_wait_signal.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/scope_exit.hpp>

#include <iostream>
#include <cstdio>


int main() {
    boost::asio::io_context context;
    asio_coro::spawn_coroutine(context, [&context]() -> asio_coro::task<void> {
        boost::asio::ip::tcp::acceptor acceptor(context);
        boost::asio::ip::tcp::endpoint acceptor_endpoint(boost::asio::ip::make_address_v4(0u), 62000);

        boost::system::error_code open_error;
        acceptor.open(acceptor_endpoint.protocol(), open_error);
        if (open_error) {
            std::cout << "open failed: " << open_error.message() << std::endl;
            co_return;
        }

        boost::system::error_code bind_error;
        acceptor.bind(acceptor_endpoint, bind_error);
        if (bind_error) {
            std::cout << "bind failed: " << bind_error.message() << std::endl;
            co_return;
        }

        boost::system::error_code listen_error;
        acceptor.listen(5, listen_error);
        if (listen_error) {
            std::cout << "listen failed: " << listen_error.message() << std::endl;
            co_return;
        }

        std::cout << "listening for incoming connections on " << acceptor_endpoint << std::endl;

        while (true) {
            boost::asio::ip::tcp::socket socket(context);
            const auto accept_error = co_await asio_coro::async_accept(acceptor, socket);
            if (accept_error) {
                std::cout << "failed to accept an incoming connection: "
                          << accept_error.message()
                          << std::endl;
                break;
            }

            std::cout << "an incoming connection accepted from " << socket.remote_endpoint() << std::endl;

            // Start a ping-pong routine for the newly accepted connection.
            asio_coro::spawn_coroutine(context, [socket = std::move(socket)]() mutable -> asio_coro::task<void> {
                const auto remote_endpoint = socket.remote_endpoint();
                while (true) {
                    std::uint8_t buffer[1024];
                    const auto [read_error, bytes_read] = co_await asio_coro::async_read(socket,
                            boost::asio::mutable_buffer(buffer, 1024));
                    if (read_error) {
                        std::cout << "failed to read data from " << remote_endpoint << ": " << read_error.message()  << std::endl;
                        break;
                    } else {
                        std::cout << "read " << bytes_read << " from " << remote_endpoint << std::endl;
                    }

                    const auto [write_error, bytes_written] = co_await asio_coro::async_write(socket,
                            boost::asio::buffer(buffer, bytes_read));
                    if (write_error) {
                        std::cout << "failed to write data to " << remote_endpoint << ": " << write_error.message() << std::endl;
                        break;
                    } else {
                        std::cout << "wrote " << bytes_written << " to " << remote_endpoint << std::endl;
                    }
                }
            });
        }
    });

    asio_coro::spawn_coroutine(context, [&context]() -> asio_coro::task<void> {
        BOOST_SCOPE_EXIT_ALL(&) {
            context.stop();
        };

        boost::asio::signal_set signal_set(context, SIGINT, SIGTERM);
        while (true) {
            const auto[wait_error, signal] = co_await asio_coro::async_wait_signal(signal_set);
            if (wait_error) {
                std::cout << "failed to wait for signal: " << wait_error.message() << std::endl;
                co_return;
            }

            if (signal == SIGINT) {
                std::cout << "SIGINT received, stopping the io context" << std::endl;
                break;
            } else if (signal == SIGTERM) {
                std::cout << "SIGTERM received, stopping the io context" << std::endl;
                break;
            }
        }
    });

    context.run();

    return 0;
}
