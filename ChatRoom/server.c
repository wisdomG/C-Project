#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#define USER_LIMIT 2
#define BUFFER_SIZE 64
#define FD_LIMIT 65535  /* 文件描述符数量限制 */

/* 客户端信息 */
struct client_data {
    sockaddr_in address;
    char *write_buf;        // 写出数据
    char buf[BUFFER_SIZE];  // 数据读到buf中
};

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    /* inet_pton将字符串形式的IP地址转换成uint32形式的 */
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    client_data *users = new client_data[FD_LIMIT];
    /* 共计USER_LIMIT+1个文件描述符，+1是因为还有一个listenfd */
    pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;

    for (int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;   // 由后续的connfd来填充
        fds[i].events = 0;
    }
    fds[0].fd = listenfd;
    /* listenfd上的两个事件，可读事件和错误事件 */
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while(1) {
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0) {
            printf("poll failed\n");
            break;
        }
        /* 遍历这几个文件描述符 */
        for (int i = 0; i < user_counter + 1; ++i) {
            /* listenfd上的读事件，说明有新的连接 */
            if ((fds[i].fd == listenfd) && (fds[i].revents & POLLIN)) {
                /* sockaddr_in是IPv4的专用地址结构 */
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)(&client_address), &client_addrlength);
                if (connfd < 0) {
                    printf("error is : %d\n", errno);
                    continue;
                }
                if (user_counter >= USER_LIMIT) {
                    const char *info = "too many users\n";
                    printf("%s\n", info);
                    /* 发送提示信息，然后关闭企图连接的文件描述符 */
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address;
                setnonblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("comes a new user, now hava %d users\n", user_counter);
            }
            else if(fds[i].revents & POLLERR) {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                socklen_t len = sizeof(errors);
                memset(errors, '\0', len);
                /* getsockopt获取socket文件描述符的信息 */
                /* 获取并清除socket文件描述符上的错误状态，即清除本次错误，下次可以继续执行 */
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            }
            else if (fds[i].revents & POLLRDHUP) {
                /* 客户机关闭连接 */
                /* 把最后一个fd拿过来填充到要被关闭的fd位置，然后再将counter--，i--，
                 * 以便下次遍历还能遍历到到这个文件描述符 */
                users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                /* i--,cnt++是为了其 */
                i--;
                user_counter--;
                printf("a client left\n");
            }
            else if (fds[i].revents & POLLIN) {
                /* 文件描述符上有数据可读 */
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data %s from %d\n", ret, users[connfd].buf, connfd);
                if (ret < 0) { // 数据接收出错
                /* EAGAIN说明数据太多报错，实际不影响程序，由于是LT，下次正常接收即可 */
                    if (errno != EAGAIN) { // != EAGAIN说明镇的出错
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if (ret == 0) { // 说明是客户端调用close主动关闭了连接

                }
                else {
                    /* 通知其他socket准备写数据，即将某个socket发过来的数据
                     * 都发送给其余几个socket */
                    for (int j = 1; j <= user_counter; ++j) {
                        if (fds[j].fd == connfd) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN; // 关闭读事件
                        fds[j].events |= POLLOUT; // 打开写事件
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if (fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if (!users[connfd].write_buf) {
                    continue;
                }
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[] users;
    close(listenfd);
    return 0;
}
