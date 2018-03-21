#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "server_base.h"
using namespace std;

namespace WebServer {
    typedef boost::asio::ip::tcp::socket HTTP;

    template<>
        class Server<HTTP> : public ServerBase<HTTP> {
            public:
                Server(unsigned short port, size_t num_threads=1):
                    ServerBase<HTTP>::ServerBase(port, num_threads) {};
            private:
                void accept() {
                    auto socket = make_shared<HTTP>(m_io_service);
                    acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
                        accept();
                        if (!ec) process_request_and_respond(socket);
                            });
                }
        };
}

#endif


