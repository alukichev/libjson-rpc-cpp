/**
 * @file
 * @date 21.03.2013
 * @author Alexander Lukichev <alexander.lukichev@gmail.com>
 * @brief This is a simple tcp client example.
 */

#include <cstdlib>
#include <iostream>

#include <jsonrpc/rpc.h>

using namespace jsonrpc;
using namespace std;

int main()
{
    Client c(new TcpClient("tcp://localhost:8889"), true);

    Json::Value params;

    params[0] = 1;

    for (int i = 0; i < 100; ++i) {
        try
        {
            cout << c.CallMethod("SCC.ping", params) << endl;
        }
        catch (const Exception& e)
        {
            cerr << e.what() << endl;
        }

        ::sleep(1);
    }

    return 0;
}
