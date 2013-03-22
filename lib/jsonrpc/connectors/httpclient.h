/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    httpclient.h
 * @date    02.01.2013
 * @author  Peter Spiess-Knafl <peter.knafl@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef HTTPCLIENT_H_
#define HTTPCLIENT_H_

/* Prevent direct inclusion if support not configured */
#ifdef LIBJSON_RPC_CPP_BUILD
#include "config.h"
#else
#include <jsonrpc/config.h>
#endif

#if !LIBJSON_RPC_CPP_CONFIG_USE_HTTP

#warning HTTP support is not configured

#else /* LIBJSON_RPC_CPP_CONFIG_USE_HTTP */

#include "../clientconnector.h"
#include "../exception.h"
#include <curl/curl.h>

namespace jsonrpc
{
    
    class HttpClient : public AbstractClientConnector
    {
        public:
            HttpClient(const std::string& url) throw (Exception);
            virtual ~HttpClient();

            virtual std::string SendMessage(const std::string& message) throw (Exception);

            void SetUrl(const std::string& url);

        private:
            std::string url;
            CURL* curl;
    };

} /* namespace jsonrpc */

#endif /* LIBJSON_RPC_CPP_CONFIG_USE_HTTP */

#endif /* HTTPCLIENT_H_ */
