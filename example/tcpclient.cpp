/**
 * @file
 * @date 21.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief This is a simple tcp client example.
 */

#include <jsonrpc/rpc.h>
#include <iostream>

using namespace jsonrpc;
using namespace std;

int main()
{
    Client c(new TcpClient("tcp://localhost:8889"), true);

    Json::Value params;

    params[0] = 1;

    try
    {
        cout << c.CallMethod("SCC.ping", params) << endl;
    }
    catch (const Exception& e)
    {
        cerr << e.what() << endl;
    }


}
