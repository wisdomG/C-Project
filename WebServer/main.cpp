#include "server_base.h"
#include "http_server.h"
#include "handler.h"
using namespace WebServer;

int main() {
    Server<HTTP> server(12345, 4);
    start_server<Server<HTTP>>(server);
    return 0;
}
