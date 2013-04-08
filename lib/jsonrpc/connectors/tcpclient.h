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
            /* If one uses the default constructor, SetUrl() must be called before sending a message. */
            TcpClient(void);
            TcpClient(unsigned int response_buf_size = 4096) throw (Exception);
            TcpClient(const std::string& url, unsigned int response_buf_size = 4096) throw (Exception);
            virtual ~TcpClient();

            virtual std::string SendMessage(const std::string& message) throw (Exception);

            bool SetUrl(const std::string& url = "localhost");

        private:
            TcpClientPrivate *_d;

        private:
            void _init(const std::string& url, unsigned int response_buf_size) throw (Exception);
            bool _connect(void);
            void _disconnect(void);
    };

} /* namespace jsonrpc */

#endif /* TCPCLIENT_H_ */
