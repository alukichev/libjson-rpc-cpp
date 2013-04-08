/**
 * @file tcpclient.h
 * @date 20.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief TCP-based ClientConnector implementation
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include "../clientconnector.h"
#include "../exception.h"

namespace jsonrpc
{
    struct TcpClientPrivate;

    class TcpClient : public AbstractClientConnector
    {
        public:
            TcpClient();
            TcpClient(const std::string& url) throw (Exception);
            virtual ~TcpClient();

            virtual std::string SendMessage(const std::string& message) throw (Exception);

            bool SetUrl(const std::string& url = "localhost");

        private:
            TcpClientPrivate *_d;

        private:
            bool _connect(void);
            void _disconnect(void);
    };

} /* namespace jsonrpc */

#endif /* TCPCLIENT_H_ */
