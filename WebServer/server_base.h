#ifndef _SERVER_BASE_H_
#define _SERVER_BASE_H_

#include <unordered_map>
#include <iostream>
#include <regex>
#include <map>
#include <string>
#include <thread>
#include <boost/asio.hpp>
using namespace std;

namespace WebServer {

    struct Request {
        // http请求行中的请求方法，请求路径和http版本
        string method, path, http_version;
        // 对content使用智能指针进行引用计数
        shared_ptr<istream> content;
        // 请求头的字典
        unordered_map<string, string> header;
        // 正则处理路径匹配问题
        smatch path_match;
    };

    // 定义资源类型，是一个map，key是一个字符串(在本项目中其实一个正则表达式，匹配用户请求资源的路径)
    // value是一个hashmap，key是字符串(表示请求的方法，GET或者POST)，
    // value是一个返回类型为空，参数为ostream和Request的函数(响应给用户的内容，包括请求头信息与输出数据流)
    // 就相当于是一个响应体和响应头的结合
    typedef map<string, unordered_map<string,
            function<void(ostream&, Request&)>>> resource_type;

    template<typename socket_type>
    class ServerBase {
        public:
            // 用于服务器访问资源处理方式
            resource_type resource;
            // 用于保存默认资源的处理方式
            resource_type default_resource;
            // 启动服务器
            void start();

            ServerBase(unsigned short port, size_t num_threads = 1) :
                endpoint(boost::asio::ip::tcp::v4(), port), // 这里对endpoint进行了构造
                acceptor(m_io_service, endpoint),           // 这里对acceptor进行了构造
                num_threads(num_threads) {}
        protected:
            // 用于内部实现对所有资源的处理
            vector<resource_type::iterator> all_resources;

            boost::asio::io_service m_io_service;
            boost::asio::ip::tcp::endpoint endpoint;
            boost::asio::ip::tcp::acceptor acceptor;

            // 设计线程池，单线程的web服务器显然是不可行的
            size_t num_threads;
            vector<thread> threads;

            // 根据不同类型的服务器实现不同的方法
            // 实际上就是交给子类去实现
            virtual void accept() {}
            // 处理请求和应答
            void process_request_and_respond(shared_ptr<socket_type> socket) const;
            // 解析请求
            Request parse_request(istream& stream) const ;
            // 做出回应
            void respond(shared_ptr<socket_type> socket, shared_ptr<Request> request) const ;
    };

    // Server是一个子类，从父类那里继承了构造函数，因此在main函数中直接调用了带参构造函数
    // Server<HTTP> server(12345, 5)
    template<typename socket_type>
    class Server: public ServerBase<socket_type> {

    };

    // 在handler.h中的start_server函数中进行资源的准备，最后使用start函数启动服务器
    template<typename socket_type>
    void ServerBase<socket_type>::start() {
        // 在handler中的start_server函数中配置了resource
        // 将资源
        for (auto it = resource.begin(); it != resource.end(); ++it) {
            all_resources.push_back(it);
        }
        for (auto it = default_resource.begin(); it != default_resource.end(); ++it) {
            all_resources.push_back(it);
        }
        cout << "# total resource: " << all_resources.size() << endl;

        // 调用socket的连接方式，需要子类实现accept函数
        // 对于http服务器来说，使用异步接收async_accept
        accept();

        // 使用一个io_service和多个线程是一个很常见的例子，在任意一个线程中启动run方法，
        // 都能够捕获到所有的事件，在单线程模式下，多个事件之间也是顺序执行的，但在多线程模式下，
        // 多个线程可以同时处理不同的事件
        for (size_t c = 1; c < num_threads; ++c) {
            threads.emplace_back([this](){
                    m_io_service.run();
                    });
        }

        // 运行子线程
        m_io_service.run();

        // 等待其他线程的结束
        for (auto &t: threads) {
            t.join();
        }
    }

    template<typename socket_type>
    void ServerBase<socket_type>::process_request_and_respond(shared_ptr<socket_type> socket) const {
        auto read_buffer = make_shared<boost::asio::streambuf>();
        // 请求头和请求体之间会有一个空行，使用read_util读取到这个空行时结束
        // 数据被存放在read_buffer中
        // 需要说明的是，虽然是用的until，但所有数据都存放到了read_buffer中，
        // 实践可以发现read_buffer的长度和bytes_transferred是相等的
        boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
                [this, socket, read_buffer](const boost::system::error_code& ec, size_t bytes_transferred) {
                if (!ec) {
                    size_t total = read_buffer->size();
                    // 转换成输入流
                    istream stream(read_buffer.get());
                    // shared_ptr<Request>类型
                    auto request = make_shared<Request>();
                    // 解析stream中的信息，并保存到request对象中
                    *request = parse_request(stream);

                    // 这里把解析出来的请求信息打印出来
                    cout << "method: " << request->method << endl;
                    cout << "path: " << request->path << endl;
                    cout << "version: " << request->http_version << endl;
                    for (auto &p : request->header) {
                        cout << p.first << " : " << p.second << endl;
                    }

                    size_t num_additional_bytes = total - bytes_transferred;

                    if (request->header.count("Content-Length") > 0) {
                        cout << "Content Length = " << request->header["Content-Length"] << endl;
                        boost::asio::async_read(*socket, *read_buffer,
                        boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes),
                        [this, socket, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transferred) {
                            if (!ec) {
                                request->content = shared_ptr<istream>(new istream(read_buffer.get()));
                                respond(socket, request);
                            }
                        });
                    } else {
                        cout << "No Content Length element in the request" << endl;
                        respond(socket, request);
                    }
                }
            });
    }

    template<typename socket_type>
    Request ServerBase<socket_type>::parse_request(istream& stream) const {
        Request request;
        // 通过该正则表达式对请求头进行解析，可以解析出请求方法，请求路径，和HTTP版本
        // 一个Http的请求行为 POST www.baidu.com/index.html HTTP/1.1
        // ^字符串开始，$字符串结束，[^ ]不包含空格
        regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        smatch sub_match;

        string line;
        getline(stream, line);
        line.pop_back();
        if (regex_match(line, sub_match, e)) {
            // 通过子匹配获取请求行信息
            request.method = sub_match[1];
            request.path = sub_match[2];
            request.http_version = sub_match[3];

            bool matched;
            // 第一个括号匹配请求头中的key，第二个括号匹配value
            e = "^([^:]*): ?(.*)$";
            // 循环获取请求头中的键值信息
            do {
                getline(stream, line);
                line.pop_back();
                matched = regex_match(line, sub_match, e);
                if (matched) {
                    request.header[sub_match[1]] = sub_match[2];
                }
            } while (matched == true);
        }
        return request;
    }

    template<typename socket_type>
    void ServerBase<socket_type>::respond(shared_ptr<socket_type> socket, shared_ptr<Request> request) const {
        // 定位请求资源的位置
        for (auto res_it : all_resources) {
            regex e(res_it->first);
            smatch sm_res;
            if (regex_match(request->path, sm_res, e)) {
                if (res_it->second.count(request->method) > 0) {
                    request->path_match = move(sm_res);

                    auto write_buffer = make_shared<boost::asio::streambuf>();
                    // 定义了一个输出流对象
                    ostream response(write_buffer.get());
                    // 下面这一步相当于调用了函数，函数的行为在handler文件中进行了定义
                    res_it->second[request->method](response, *request);

                    // 将write_buffer上的数据返回
                    boost::asio::async_write(*socket, *write_buffer,
                            [this, socket, request, write_buffer](const boost::system::error_code& ec, size_t bytes_transferred) {
                                if (!ec && stof(request->http_version) > 1.05) {
                                    cout << "send the data: " << bytes_transferred << endl;
                                    // 继续进行异步接受request
                                    process_request_and_respond(socket);
                                }
                            });
                    return ;
                }
            }
        }
    }
};


#endif
