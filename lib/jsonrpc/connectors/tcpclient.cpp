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
        char *buffer;
        size_t buffer_size;

        inline TcpClientPrivate(size_t bufsize, const std::string& url) : ioService(), socket(ioService) { setUrl(url); createBuf(bufsize); }
        inline TcpClientPrivate(size_t bufsize) : ioService(), socket(ioService) { createBuf(bufsize); }

        inline ~TcpClientPrivate() { if (buffer) delete buffer; }

        inline void createBuf(size_t size);
        inline void setUrl(const std::string& url);

    private:
        static const std::string _DefaultPort;
        static const size_t _DefaultBufsize;
    };

    const std::string TcpClientPrivate::_DefaultPort = "8889";
    const size_t TcpClientPrivate::_DefaultBufsize = 4096;

    inline void TcpClientPrivate::createBuf(size_t size)
    {
        if (size <= 1) {
            DOUT("specified %i as buffer size, setting to default size %i",
                 size, _DefaultBufsize);
            size = _DefaultBufsize;
        }

        buffer = NULL;
        buffer_size = 0;

        try { // If new throws an exception
            buffer = new char[size];
            buffer_size = size;
        }
        catch (const std::exception& e) {
            DOUT("could not allocate a buffer of %i bytes: %s", e.what());
        }
    }

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

    TcpClient::TcpClient(unsigned int response_buf_size) throw (Exception)
    {
        DOUT("Creating a connector without url");
        _d = new TcpClientPrivate(response_buf_size);
        if (!_d || !_d->buffer) {
            if (_d) {
                delete _d;
                _d = NULL;
            }

            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not get memory");
        }
    }

    TcpClient::TcpClient(const std::string& url, unsigned int response_buf_size) throw (Exception)
    {
        _d = new TcpClientPrivate(response_buf_size, url);
        if (!_d || !_d->buffer) {
            if (_d) {
                delete _d;
                _d = NULL;
            }

            throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not get memory");
        }
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
            // NOTE: a very dirty way to get enough data from a socket is to read it all in one
            //       operation into a sufficiently large buffer. This is done here as I could
            //       not figure out how to get the size of available data on a boost::async socket.
            //       For now, this large buffer approach should be enough for my purposes but this
            //       limitation stays:
            //       1. TcpClient is not able to handle responses larger than its buffer.
            //       2. Any data following a proper response (e.g. a notification) will be
            //          silently discarded by AbstractClient code.
            //
            //       It does not matter anyway because we have already decided to move away from
            //       using libjsoncpp (not very portable, no support for notifications,
            //       unportable dependency - libjson, unstable API).
            boost::system::error_code error;
            size_t len;

            len = _d->socket.write_some(boost::asio::buffer(message.c_str(), message.size()), error);
            if (error && error != boost::asio::error::eof)
                throw system_error(error);
            if (len < message.size())
                throw Exception(Errors::ERROR_CLIENT_CONNECTOR, "could not send request");

            len = _d->socket.read_some(boost::asio::buffer(_d->buffer, _d->buffer_size), error);
            if (error && error != boost::asio::error::eof)
                throw system_error(error);

            response.assign(_d->buffer, len);
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
