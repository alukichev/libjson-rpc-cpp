/**
 * @file tcpclient.h
 * @date 20.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief TCP-based ClientConnector implementation
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <boost/asio.hpp>

#include "../clientconnector.h"
#include "../exception.h"

namespace jsonrpc
{

    class TcpClient : public ClientConnector
    {
        public:
            TcpClient(const std::string& url);
            virtual ~TcpClient();

            virtual std::string SendMessage(const std::string& message) throw (Exception);

        private:
            const std::string _url;
            boost::asio::io_service _ioService;
            boost::asio::ip::tcp::socket _socket;

        private:
            bool _connect(void);
    };

} /* namespace jsonrpc */

#endif /* TCPCLIENT_H_ */
