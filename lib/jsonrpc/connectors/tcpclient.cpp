/**
 * @file
 * @date 21.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief TCP-based client connector.
 */

#include "../exception.h"
#include "tcpclient.h"

#include <cstdio>
#include <iostream>
#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <errno.h>
#include <string.h>

using namespace std;
using boost::asio::ip::tcp;

#define DEBUG

#ifdef DEBUG
#define DOUT(msg, ...)   ({fprintf(stderr, "%s:%d %s(): " msg "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);})
#else
#define DOUT(msg, ...)
#endif

namespace jsonrpc
{
    struct TcpClientPrivate {
        const std::string url;
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::socket socket;

        inline TcpClientPrivate(const std::string& url_) : url(url_), ioService(), socket(ioService) {}
    };

    TcpClient::TcpClient(const std::string& url) throw (Exception)
    {
        _d = new TcpClientPrivate(url);
        if (!_d)
            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not get memory");
    }

    TcpClient::~TcpClient(void)
    {
        if (_d)
            delete _d;
    }

    std::string TcpClient::SendMessage(const std::string& message) throw (Exception)
    {
        if (!_connect())
            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "url:" + _d->url);

        std::string response;

        DOUT("sending request %s", message.c_str());

        try
        {
            boost::array<char, 1024> buf;
            boost::system::error_code error;
            size_t len;

            len = _d->socket.write_some(boost::asio::buffer(message.c_str(), message.size()), error);
            if (error && error != boost::asio::error::eof)
                throw boost::system::system_error(error);
            if (len < message.size())
                throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not send request");

            len = _d->socket.read_some(boost::asio::buffer(buf), error);
            if (error && error != boost::asio::error::eof)
                throw boost::system::system_error(error);

            response = buf.data();
            DOUT("got response %s", response.c_str());
        }
        catch (const std::exception& e)
        {
            DOUT("error %s", e.what());
            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, e.what());
        }

        return response;
    }

    bool TcpClient::_connect()
    {
        if (_d->socket.is_open())
            return true;

        try
        {
            tcp::endpoint server(boost::asio::ip::address_v4::from_string("127.0.0.1"), 8889);
            boost::system::error_code error;

            _d->socket.connect(server, error);
            if (error)
                throw boost::system::system_error(error);
        }
        catch (const std::exception& e)
        {
            DOUT("could not connect to 127.0.0.1:8889: %s", e.what());
            return false;
        }

        DOUT("connected to 127.0.0.1:8889");

        return true;
    }
}
