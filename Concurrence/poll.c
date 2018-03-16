#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>

/*poll
 * client是一个pollfd类型的数组，存储被监听的文件描述符，当poll函数返回后，
 * 检查数组中每个元素的revent是否置位，是则进行后续操作
 */

const int SERV_PORT = 8000;
const int LEN = 4096;
// 设置最大监听数为OPEN_MAX
const int OPEN_MAX = 1024;

int main() {
    int listenfd, connfd, sockfd;
    struct pollfd client[OPEN_MAX]; // 默认1024
    int nready;
    char buf[LEN];
    char str[INET_ADDRSTRLEN];

    struct sockaddr_in serv_addr, client_addr;

    // 启动监听
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);
    bind(listenfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr));
    listen(listenfd, 20);

    // 将listenfd放入client中管理，并设置数据可读事件
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;

    int i = 1, maxi = 0;  // client中有效元素的最大下标
    for (i = 1; i < OPEN_MAX; ++i)
        client[i].fd = -1;

    printf("监听文件描述符:%d\n", listenfd);

    for (; ;) {
        nready = poll(client, maxi+1, -1); // 阻塞监听
        if (nready < 0) {
            perror("poll error");
            exit(1);
        }

        if (client[0].revents & POLLRDNORM) { // 监听到一个新连接
            socklen_t client_addr_len;
            client_addr_len = sizeof(client_addr);
            connfd = accept(listenfd, (struct sockaddr*)(&client_addr), &client_addr_len);
            printf("监听到一个新连接，%s:%d，文件描述符为%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port), connfd);

            for (i = 1; i < OPEN_MAX; ++i) {
                if (client[i].fd < 0) {     //找到空闲位置，填入connfd
                    client[i].fd = connfd;
                    break;
                }
            }
            if (i == OPEN_MAX) {
                perror("Too many clients\n");
                exit(1);
            }
            client[i].events = POLLRDNORM;

            if (i > maxi) maxi = i;
            if (--nready == 0) continue;
        }

        // 查看其他有消息过来的文件描述符，准备开始通信
        for (int i = 1; i <= maxi; ++i) {
            if ((sockfd = client[i].fd) < 0)
                continue;
            // 检查revents是否置位
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                ssize_t n;
                if ((n = read(sockfd, buf, LEN)) == 0) {
                    close(sockfd);
                    client[i].fd = -1;
                    printf("客户端 %d 关闭连接\n", sockfd);
                } else if (n > 0){
                    buf[n] = '\0';
                    printf("来自文件描述符 %d 的消息：%s\n", sockfd, buf);
                    int j ;
                    for (j = 0; j < n; ++j)
                        buf[j] = toupper(buf[j]);
                    send(sockfd, buf, n, 0);
                } else if (n < 0) {
                    if (errno == ECONNRESET) {
                        close(sockfd);
                        client[i].fd = -1;
                        printf("客户端 %d 意外终止连接\n", sockfd);
                    } else {
                        perror("read error");
                        exit(1);
                    }
                }

                if (--nready == 0)
                    break;
            }
        }
    }

    close(listenfd);

    return 0;
}
