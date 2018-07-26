#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

/* select
 * 用client保存每个处于被监听状态的fd，当select函数返回后，遍历client中的每个fd，
 * 判断其在fd_set中是否处于set状态，是则可以进行后续处理
 */

// 设置端口号
const int SERV_PORT = 8000;
const int LEN = 4096;

int main() {
    int listenfd, connfd, sockfd;
    int client[FD_SETSIZE]; // FD_SETSIZE默认1024，保存处于被监听状态的文件描述符
    int nready;             // 监听到的数量
    char buf[LEN], str[INET_ADDRSTRLEN];
    struct sockaddr_in serv_addr, client_addr;
    fd_set rset, allset;    // 文件描述符集fd_set是一个long型数组，每个元素都与一个句柄建立联系

    // 设置socket相关参数并开始监听
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);
    bind(listenfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr));
    listen(listenfd, 20);

    int maxfd = listenfd;
    int maxi = 0, i;

    client[0] = listenfd;
    for (i = 1; i < FD_SETSIZE; ++i)
        client[i] = -1;

    FD_ZERO(&allset);           // 将allset清零使集合中不含任何fd
    FD_SET(listenfd, &allset);  // 将listenfd加入allset
    printf("socket监听文件描述符:%d\n", listenfd);

    // allset记录所有被监听的位都为1，而rset中只有那些有数据的文件描述符对应的位为1
    for (; ;) {
        rset = allset;
        //FD_ZERO(&rset);
        // 调用select函数阻塞监听
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            printf("select error");
            exit(1);
        }

        if (FD_ISSET(listenfd, &rset)) { // 监听到一个新连接
            socklen_t client_addr_len = sizeof(client_addr);
            connfd = accept(listenfd, (struct sockaddr*)(&client_addr), &client_addr_len);
            printf("监听到一个新连接，%s:%d，文件描述符为%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port), connfd);

            // 在client中找到一个位置，加入进去
            for (i = 0; i <= FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                perror("Too many clients\n");
                exit(1);
            }

            // 加入allset进行监控
            FD_SET(connfd, &allset);

            // 更新maxfd和maxi
            if (connfd > maxfd) maxfd = connfd;
            if (i > maxi) maxi = i;

            if (--nready == 0) continue;
        }

        // 查看其他有消息过来的文件描述符，准备开始通信
        for (int i = 0; i <= maxi; ++i) {
            if ((sockfd = client[i]) < 0) // 赋值的同时检查是否为-1，是则直接跳过
                continue;

            if (FD_ISSET(sockfd, &rset)) {
                ssize_t n;
                // 返回0，说明客户端请求关闭连接，即发送了FIN信号
                if ((n = read(sockfd, buf, LEN)) == 0) {
                    strcpy(buf, "please waiting for 5 seconds\n");
                    //sleep(5);
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                    printf("客户端 %d 关闭连接\n", sockfd);
                } else {
                    buf[n] = '\0';
                    printf("来自文件描述符 %d 的消息：%s\n", sockfd, buf);
                    sleep(1);
                    // 转换成大写返回
                    for (int j = 0; j < n; ++j)
                        buf[j] = toupper(buf[j]);
                    send(sockfd, buf, n, 0);
                }

                if (--nready == 0)
                    break;
            }
        }
    }

    close(listenfd);

    return 0;
}
