#include <iostream>
#include <cctype>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

using namespace std;
using namespace boost::asio;

typedef boost::shared_ptr<ip::tcp::socket> sock_ptr;

// 如何处理客户端的断开
void client_session(sock_ptr sock) {
    while(true) {
        char buf[512];
        // 两种捕获错误的方式，用于客户端断开连接后的处理
        /* 一种是利用重载，传入一个error_code
        boost::system::error_code ec;
        size_t len = sock->read_some(buffer(buf), ec);
        if (ec) {
            sock->close();
            return ;
        }*/
        // 另一个是用try catch捕获一个system_error
        // 注意两者是不一样的，一个是error_code，一个是system_error
        size_t len;
        try {
            cout << "blocking to read data\n";
            //len = sock->read_some(buffer(buf));
            len = read(*sock, buffer(buf));
            cout << "read the data" << endl;
        } catch (boost::system::system_error e){
            cout << e.code() << endl; // 通过code方法获取code
            sock->close();
            return ;
        }

        if (len > 0) {
            buf[len] = '\0';
            for (int i = 0; i < len; ++i)
                buf[i] = toupper(buf[i]);
            write(*sock, buffer(buf, len));
            //sock->write(buffer(buf, len));
        }
    }
    sock->close();
    return ;
}


int main() {
    //using boost::asio;
    io_service service;
    //ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8000);
    ip::tcp::endpoint ep(ip::tcp::v4(), 8000);
    ip::tcp::acceptor acc(service, ep);
    while (true) {
        // 准备一个sock
        //typedef boost::shared_ptr<ip::tcp::socket> sock_ptr;
        sock_ptr sock(new ip::tcp::socket(service));
        cout << "We have prepared a sock_ptr \n";
        cout << "Blocking...." << endl;
        acc.accept(*sock);
        boost::thread(boost::bind(client_session, sock));
    }
    return 0;
}
