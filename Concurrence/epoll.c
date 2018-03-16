#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <sys/epoll.h>
#include <errno.h>

/* epoll
 * 使用epoll模型时，将被监听的事件加入efd这个文件描述符中进行管理，调用epoll_wait后，
 * 满足要求的事件就进入到一个队列中（对应代码中的ep），然后只需遍历这个队列即可
 * 这里的client并没有什么用，只是记录一下被监听的fd，防止监听个数超限
 */

const int SERV_PORT = 8000;
const int LEN = 80;
const int OPEN_MAX = 1024;

int main() {
    int listenfd, maxfd, connfd, sockfd;
    int efd, res;
    int nready;
    char buf[LEN];
    char str[INET_ADDRSTRLEN];

    int client[OPEN_MAX];
    struct epoll_event tep, ep[OPEN_MAX];
    struct sockaddr_in serv_addr, client_addr;

    // 启动监听
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);
    bind(listenfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr));
    listen(listenfd, 20);

    client[0] = listenfd;
    int maxi = 0, i, j;
    for (i = 1; i < OPEN_MAX; ++i)
        client[i] = -1;

    // 新建一个用于epoll的文件描述符
    efd = epoll_create(OPEN_MAX);
    if (efd == -1) {
        printf("epoll create error");
        exit(1);
    }

    // 设置listenfd对应事件类别并加入到efp中进行管理
    tep.events = EPOLLIN;
    tep.data.fd = listenfd;
    res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    if (res == -1) {
        perror("opoll control error\n");
        exit(1);
    }

    printf("监听文件描述符:%d\n", listenfd);

    for (; ;) {
        nready = epoll_wait(efd, ep, OPEN_MAX, -1); // 阻塞监听
        if (nready < 0) {
            perror("epoll wait error\n");
            exit(1);
        }

        // epoll_wait返回后，ep中存放了所有满足要求的文件描述符
        // 此时只需要遍历ep就行了，无需遍历client，因此提高了效率
        for (i = 0; i < nready; ++i) {
            if (!(ep[i].events & EPOLLIN))
                continue;
            if (ep[i].data.fd == listenfd) {   // 处理socket连接请求
                socklen_t client_addr_len;
                client_addr_len = sizeof(client_addr);
                connfd = accept(listenfd, (struct sockaddr*)(&client_addr), &client_addr_len);
                printf("监听到一个新连接，%s:%d，文件描述符为%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port), connfd);

                for (j = 0; j < OPEN_MAX; ++j)
                    if (client[j] == -1) {
                        client[j] = connfd;
                        break;
                    }
                if (j == OPEN_MAX) {
                    perror("Too many clients\n");
                    exit(1);
                }
                if (j > maxi) maxi = j;
                tep.events = EPOLLIN;
                tep.data.fd = connfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                if (res == -1) {
                    perror("opoll add new connection error\n");
                    exit(1);
                }

            } else { // 处理socket上的数据传入
                sockfd = ep[i].data.fd;
                ssize_t n;
                if ((n = read(sockfd, buf, LEN)) == 0) {
                    // 将client中对应的位置置为-1
                    for (j = 0; j <= maxi; ++j) {
                        if (client[j] == sockfd) {
                            client[j] = -1;
                            break;
                        }
                    }
                    // 关闭时先将对应的fd从ep中删除，然后再关系socket
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    if (res == -1) {
                        perror("delete error, n == 0\n");
                        exit(1);
                    }
                    close(sockfd);
                    printf("客户端 %d 关闭连接\n", sockfd);
                } else if (n > 0){
                    buf[n] = '\0';
                    printf("来自文件描述符 %d 的消息：%s\n", sockfd, buf);
                    int j ;
                    for (j = 0; j < n; ++j)
                        buf[j] = toupper(buf[j]);
                    send(sockfd, buf, n, 0);
                }
            }

        }
    }

    // 这里也要关闭efd这个文件描述符
    close(efd);
    close(listenfd);

    return 0;
}
