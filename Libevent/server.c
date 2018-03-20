#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const int PORT = 8000;
const char* MESSAGE = "hello world\n";
const int LEN = 4096;

static void err_exit(const char *str) {
    perror(str);
    exit(1);
}

// 在我们的通信例子中，该回调函数可以不用，调用bufferevent_write函数后就将数据发送出去了
// 从某种意义上来说，监听写事件是没有必要的，实践也证明，甚至在enable函数中关闭写监听，也不影响我们通过bufferevent_write发送数据
static void conn_write_cb(struct bufferevent* bev, void* user_data) {
    printf("send the response\n");
    /*
    struct evbuffer* output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
        printf("flushed answer\n");
        //bufferevent_free(bev);
    }*/
}

// 读回调函数
static void conn_read_cb(struct bufferevent* bev, void* user_data) {
    printf("receive message:\n");
    //struct evbuffer* input = bufferevent_get_input(bev);
    char msg[LEN];
    size_t len = bufferevent_read(bev, msg, sizeof(msg));
    msg[len] = '\0';
    printf("%s\n", msg);

    bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

// 错误事件回调函数
static void conn_event_cb(struct bufferevent* bev, short events, void* user_data) {
    printf("enter the error callback function\n");
    if (events & BEV_EVENT_EOF) {
        // 客户端主动关闭了连接
        printf("connection closed\n");
    } else if (events & BEV_EVENT_ERROR) {
        printf("got error on the connection %s\n", strerror(errno));
    }
    // 该free函数会自动关闭sockfd和和相关的缓冲区
    bufferevent_free(bev);
}

static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, \
    struct sockaddr* sa, int socklen, void* user_data) {
    printf("======= this is listener callback and the fd = %d\n", fd);
    struct event_base* base = user_data;
    struct bufferevent* bev;

    // 新建一个bufferevent关联到socketfd，托管到base
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        perror("error constructing bufferevent\n");
        event_base_loopbreak(base);
        return ;
    }
    // 设置对应的回调函数
    // 参数2/3/4分别对应该buffer上的读回调，写回调和错误回调
    bufferevent_setcb(bev, conn_read_cb, NULL, conn_event_cb, NULL);
    // 启动监听读写事件，其实就相当于event_add
    bufferevent_enable(bev, EV_READ);
    // bufferevent_enable(bev, EV_READ|EV_WRITE);
    const char *msg = "receive a new connection\n";
    bufferevent_write(bev, msg, strlen(msg));
}

// 收到ctrl+C信号，两秒后退出事件循环
static void signal_cb(evutil_socket_t sig, short events, void* user_data) {
    struct event_base *base = user_data;
    struct timeval delay = {2, 0};
    printf("exit after 2 seconds\n");
    event_base_loopexit(base, &delay);
}

int main(int argc, char **argv) {
    struct event_base* base;
    struct evconnlistener* listener;
    struct event* signal_event;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) err_exit("could not initialize event_base\n");

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    // evconnlistener_new_bind 函数帮我们完成了socket bind listen accpet 四个操作
    // para1：event_base*
    // para2: 函数指针，该函数自动传入一系列参数，包括listener指针，accpet函数返回后的socket文件描述符和sockaddr结构体等
    // para3：用户参数
    // para4：标志，第一个参数表明该socket释放后，它所使用的端口能立即被其他socket使用，第二个参数表示监听释放后，自动关闭底层socket
    // para5：listen的第二个参数
    // para6 & para7：不多说
    listener = evconnlistener_new_bind(base, listener_cb, (void*)base,
            LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
            (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) err_exit("could not create listener\n");

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);
    if (!signal_event) err_exit("could not create a signal event\n");

    if(event_add(signal_event, NULL) < 0) err_exit("could not add the signal event\n");

    event_base_dispatch(base);


    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}
