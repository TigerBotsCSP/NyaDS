#pragma once
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ssl.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace ssl = boost::asio::ssl;

class ws {
public:
    ws(asio::io_context& io_ctx, const std::string& host, const std::string& port)
        : resolver_(io_ctx)
        , ws_(io_ctx)
        , host_(host)
        , port_(port)
        , ssl_ctx_(ssl::context::sslv23_client)
    {
        // Set SSL options and turn off certificate verification for simplicity
        ssl_ctx_.set_default_verify_paths();
        ssl_ctx_.set_verify_mode(ssl::verify_none);
    }

    bool connect()
    {
        try {
            // Resolve the host name and port number to an endpoint
            auto endpoints = resolver_.resolve(host_, port_);

            // Connect to the endpoint
            asio::connect(ws_.next_layer().next_layer(), endpoints);

            // Perform SSL handshake
            ws_.next_layer().handshake(ssl::stream_base::client);

            // Perform WebSocket handshake
            ws_.handshake(host_, "/");

            return true;
        }
        catch(std::exception &e) {
            return false;
        }
    }

    void send(const std::string& message)
    {
        ws_.write(asio::buffer(message));
    }

public:
    asio::ip::tcp::resolver resolver_;
    websocket::stream<ssl::stream<asio::ip::tcp::socket>> ws_;
    std::string host_;
    std::string port_;
    ssl::context ssl_ctx_;
};
