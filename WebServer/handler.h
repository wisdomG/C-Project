#ifndef _HANDLER_H_
#define _HANDLER_H_

#include "server_base.h"
#include <fstream>

using namespace std;
using namespace WebServer;

// 这里定义了一个模板函数，程序相当于从这里启动
template<typename server_type>
void start_server(server_type &server) {
    // 准备系统启动所需的各项资源
    server.resource["^/string/?$"]["POST"] = [](ostream& response, Request& request) {
        stringstream ss;
        *request.content >> ss.rdbuf();
        // 将请求内容读取到content中
        string content = ss.str();

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };

    server.resource["^/info/?$"]["GET"] = [](ostream& response, Request& request) {
        stringstream content_stream;
        content_stream << "<h1>Request:</h1>";
        content_stream << request.method << " " << request.path << " HTTP/" << request.http_version << "<br>";
        for (auto &header: request.header) {
            content_stream << header.first << ": " << header.second << "<br>";
        }
        content_stream.seekp(0, ios::end);
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp() << "\r\n\r\n" << content_stream.rdbuf();
    };

    server.resource["^/match/([0-9a-zA-Z]+)/?$"]["GET"] = [](ostream& response, Request& request) {
        string number = request.path_match[1];
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << number.length() << "\r\n\r\n" << number;
    };

    // 处理默认GET请求，index.html
    // 应该web/ 目录及子目录中的文件
    server.default_resource["^/?(.*)$"]["GET"] = [](ostream& response, Request request) {
        string filename = "Web/";
        string path = request.path_match[1];

        size_t last_pos = path.rfind(".");
        size_t current_pos = 0;
        size_t pos;

        // 防止使用..来访问其他文件目录
        while ((pos = path.find('.', current_pos)) != string::npos && pos != last_pos) {
            current_pos = pos;
            path.erase(pos, 1);
            last_pos--;
        }

        filename += path;
        ifstream ifs;

        if (filename.find('.') == string::npos) {
            if (filename[filename.length()-1] != '/') {
                filename += '/';
            }
            filename += "index.html";
        }
        ifs.open(filename, ifstream::in);

        if (ifs) {
            ifs.seekg(0, ios::end);
            size_t length = ifs.tellg();

            ifs.seekg(0, ios::beg);
            response << "HTTP/1.1 200 OK\r\nContent_Length: " << length << "\r\n\r\n" << ifs.rdbuf();

            ifs.close();
        } else {
            string content = "Could not open file " + filename;
            response << "HTTP/1.1 400 Bad Request\r\nContent_Length: " << content.length() << "\r\n\r\n" << content;
        }
    };

    // 准备好上述资源后，启动服务器
    server.start();
}

#endif
