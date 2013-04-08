/**
 * @file
 * @date 21.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief TCP-based client connector.
 */

#include "../exception.h"
#include "tcpclient.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <errno.h>
#include <string.h>

using boost::asio::ip::tcp;
using boost::asio::io_service;
using boost::asio::ip::address_v4;
using boost::system::error_code;
using boost::system::system_error;

#define DEBUG

#ifdef DEBUG
#define DOUT(msg, ...)   ({fprintf(stderr, "%s:%d %s(): " msg "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);})
#else
#define DOUT(msg, ...)
#endif

namespace jsonrpc
{
    struct TcpClientPrivate {
        std::string host;
        std::string port;
        io_service ioService;
        tcp::socket socket;

        inline TcpClientPrivate(const std::string& url) : ioService(), socket(ioService) { setUrl(url); }
        inline TcpClientPrivate() : ioService(), socket(ioService) {}

        inline void setUrl(const std::string& url);

    private:
        static const std::string _DefaultPort;
    };

    const std::string TcpClientPrivate::_DefaultPort = "8889";

    inline void TcpClientPrivate::setUrl(const std::string &url)
    {

        // Get the host name
        const std::string prot_end = "://";
        size_t host_start = url.find(prot_end);
        if (host_start == std::string::npos)
            host_start = 0;
        else
            host_start += prot_end.length();

        const std::string host_end_s = ":/";
        size_t host_end = url.find_first_of(host_end_s, host_start);
        if (host_end == std::string::npos)
            host_end = url.length();

        size_t host_len = host_end - host_start;
        host = url.substr(host_start, host_len);

        // Get the port name
        if (url.length() <= host_end + 1 || url[host_end] != ':') {
            DOUT("url %s: assigning default port %s", url.c_str(), _DefaultPort.c_str());
            port = _DefaultPort;
        }
        else {
            char *e;
            const std::string port_s = url.substr(host_end + 1, url.length() - (host_end + 1));

            if (!::strtoul(port_s.c_str(), &e, 10) || e == port_s.c_str()) {
                DOUT("url %s has incorrect port number, assigning default %s", url.c_str(), _DefaultPort.c_str());
                port = _DefaultPort;
            }
            else {
                const size_t len = e - port_s.c_str();
                port = port_s.substr(0, len);
            }
        }

        DOUT("url %s: host %s, port %s", url.c_str(), host.c_str(), port.c_str());
    }

    TcpClient::TcpClient()
    {
        DOUT("Creating a connector without url");
        _d = new TcpClientPrivate();
    }

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
            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "url: " + _d->host + ":" + _d->port);

        std::string response;

        DOUT("sending request %s", message.c_str());

        try
        {
            boost::array<char, 1024> buf;
            boost::system::error_code error;
            size_t len;

            len = _d->socket.write_some(boost::asio::buffer(message.c_str(), message.size()), error);
            if (error && error != boost::asio::error::eof)
                throw system_error(error);
            if (len < message.size())
                throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not send request");

            len = _d->socket.read_some(boost::asio::buffer(buf), error);
            if (error && error != boost::asio::error::eof)
                throw system_error(error);

            response.assign(buf.data(), len);
            DOUT("got response %s", response.c_str());
        }
        catch (const std::exception& e)
        {
            DOUT("error %s", e.what());
            _disconnect();
            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, e.what());
        }

        return response;
    }

    bool TcpClient::SetUrl(const std::string& url)
    {
        if (!_d) {
            DOUT("not initialized");
            return false;
        }

        _d->setUrl(url);

        return !_d->host.empty();
    }

    bool TcpClient::_connect(void)
    {
        if (!_d) {
            DOUT("not initialized");
            return false;
        }

        if (_d->socket.is_open())
            return true;

        try
        {
            tcp::resolver resolver(_d->ioService);
            tcp::resolver::query query(_d->host.c_str(), _d->port.c_str());
            tcp::resolver::iterator endpoints = resolver.resolve(query);

            boost::system::error_code error = boost::asio::error::host_not_found;
            for (tcp::resolver::iterator end; error && endpoints != end; endpoints++)
            {
                const tcp::endpoint& endpoint = *endpoints;

                DOUT("Trying %s", endpoint.address().to_string().c_str());
                _d->socket.close();
                _d->socket.connect(endpoint, error);
            }
            if (error)
                throw system_error(error);
        }
        catch (const std::exception& e)
        {
            DOUT("could not connect to %s:%s: %s", _d->host.c_str(), _d->port.c_str(), e.what());
            return false;
        }

        DOUT("connected to %s:%s", _d->host.c_str(), _d->port.c_str());

        return true;
    }

    void TcpClient::_disconnect(void)
    {
        if (!_d || !_d->socket.is_open())
            return;

        try {
            boost::system::error_code error;
            _d->socket.shutdown(tcp::socket::shutdown_both, error);
            _d->socket.close();
        }
        catch (const std::exception& e) {
            DOUT("could not close socket: %s", e.what());
        }
    }
}
