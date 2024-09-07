#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <thread>
#include <memory>

namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
    explicit Session(tcp::socket socket) 
        : ws_(std::move(socket))
    {
    }

    void run()
    {
        ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
            if (!ec) self->do_read();
        });
    }

private:
    beast::websocket::stream<tcp::socket> ws_;

    void do_read()
    {
        ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec)
                return;

            std::string message = beast::buffers_to_string(self->buffer_.data());
            std::cout << "Received: " << message << std::endl;
            self->buffer_.consume(bytes_transferred);

            self->ws_.async_write(boost::asio::buffer(message), [self](beast::error_code ec, std::size_t) {
                if (ec)
                    return;

                self->do_read();
            });
        });
    }

    beast::flat_buffer buffer_;
};

class Server
{
public:
    Server(boost::asio::io_context& ioc, const std::string& ip, const std::string& port)
        : acceptor_(ioc, {boost::asio::ip::make_address(ip), static_cast<unsigned short>(std::stoi(port))})
    {
        do_accept();
    }

private:
    tcp::acceptor acceptor_;

    void do_accept()
    {
        acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (!ec)
                std::make_shared<Session>(std::move(socket))->run();

            do_accept();
        });
    }
};

int main()
{
    try
    {
        boost::asio::io_context ioc;
        Server server(ioc, "0.0.0.0", "1337");
        ioc.run();
        std::cout << "Server Started!" << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
