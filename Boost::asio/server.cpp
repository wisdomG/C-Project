#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
using namespace std;

class server {
    private:
        boost::asio::io_service m_io;
        boost::asio::ip::tcp::acceptor m_acceptor;
        shared_ptr<boost::asio::streambuf> rdbuf;
        char buf[1024];
    public:
        // acceprot这个构造函数直接完成了socket, bind, listen 等动作
        server() : m_acceptor(m_io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8000)) {
            //rdbuf = make_shared<boost::asio::streambuf>();
            accept();
        }

        void run() {
            m_io.run();
        }

        void accept() {
            // 建立一个用户客户端socket
            boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(m_io));
            cout << "a new client socket was build" << endl;
            m_acceptor.async_accept(*sock, boost::bind(&server::accept_handler, this, boost::asio::placeholders::error, sock));
            cout << "fdafas" << endl;
        }

        void accept_handler(const boost::system::error_code& ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
            accept();
            if (ec) {
                printf("error\n");
                return ;
            }
            cout << "new client: " << endl;
            cout << sock->remote_endpoint().address() << endl;
            sock->async_write(boost::asio::buffer("hello world"), boost::bind(&server::write_handler, this, boost::asio::placeholders::error));

            //boost::shared_ptr<boost::asio::streambuf> rdbuf;
            //size_t len;
            //sock->async_read(buffer(buf), boost::bind(&server::read_handler, this, _1, _2));
            cout << "ready for read" << endl;
        }

        void read_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

            cout << "receive the data" << endl;
        }

        void write_handler(const boost::system::error_code& ec) {
            cout << "send msg over" << endl;
        }
};

int main() {
    try {
        cout << "server start." << endl;
        server s;
        s.run();
        cout << "Done" << endl;
    } catch (exception &e){
        cout << e.what() << endl;
    }
    cout << "Done" << endl;
}
