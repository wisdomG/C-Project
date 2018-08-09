#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
using namespace std;

// 这里为什么大量使用共享指针呢？

// 基于BoostAsio的异步服务端

class server {
    private:
        boost::asio::io_service m_io;
        boost::asio::ip::tcp::acceptor m_acceptor;
        boost::shared_ptr<boost::asio::streambuf> rdbuf; // streambuf为什么要用智能指针呢？
        char buf[1024];
    public:
        // acceptor这个构造函数直接完成了socket, bind, listen 等动作
        server() : m_acceptor(m_io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8000)) {
            //rdbuf = make_shared<boost::asio::streambuf>();
            accept();
        }

        void run() {
            // 在run之前，给一些空闲的任务，保证run一直进行下去
            boost::shared_ptr<boost::asio::io_service::work> dummy_word(new boost::asio::io_service::work(m_io));
            m_io.run();
        }

        void accept() {
            // 建立一个用户客户端socket
            boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(m_io));
            cout << "a new client socket was build" << endl;
            // async_accept传入了socket对象，而accept_handler我们写的接受的是一个socket指针
            m_acceptor.async_accept(*sock, boost::bind(&server::accept_handler, this, boost::asio::placeholders::error, sock));
            cout << "this should not be executed" << endl;
        }

        void accept_handler(const boost::system::error_code& ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
            accept();   // 立即启动异步接受
            if (ec) {
                printf("error\n");
                return ;
            }
            cout << "new client: " << endl;
            cout << sock->remote_endpoint().address() << endl;

            // 异步写
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
