#include "server_base.h"
#include "http_server.h"
#include "handler.h"
using namespace WebServer;

int main() {
    // Server是一个模板类，HTTP是什么呢？HTTP是BoostAsio定义的socket类型
    Server<HTTP> server(12345, 4);
    // 这是一个模板函数吗？
    start_server<Server<HTTP>>(server);
    return 0;
}
