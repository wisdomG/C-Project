#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65536
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMIT 65536

struct client_data {
    sockaddr_in address; // 客户端socket地址
    int connfd;          // socket文件描述符
    pid_t pid;           // 处理这个链接的进程pid
    int pipefd[2];       // 和父进程通信用的管道
}

static const char *shm_name = "/my_shm";
int sig_pipefd[2];
int epollfd;
int listenfd;
int shmfd;
char *share_mem = 0;
client_data *users = 0;   // 客户链接数据
int *sub_process = 0;     //
int user_counter = 0;     // 当前用户个数
bool stop_child = false;

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd= fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(sig_pipfd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig, void(*handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void del_resource() {
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);
    delete[] usert;
    delete[] sub_process;
}

/* 停止子进程 */
void child_term_handler(int sig) {
    stop_child = true;
}

/************************************************************
 * 子进程运行函数
 * idx指向子进程负责的客户连接的编号
 * users保存所有客户连接数据的数组
 * share_mem指向共享内存的其实地址
 ***********************************************************/
int run_child(int idx, client_data *users, char *share_mem) {
    epoll_event events[MAX_EVENT_NUMBER];
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = uesrs[idx].connfd;
    addfd(child_epollfd, connfd);
    int pipefd = users[idx].pipefd[1];
    addfd(child_epollfd, pipefd);
    addsig(SIGTERM, child_term_handler, false);
    int ret;
    while (!stop_child) {
        int number = epoll_wait(child_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            /* 本子进程负责的客户端有数据到达 */
            if (sockfd == connfd && (events[i].events & POLLIN)) {
                memset(share_mem + idx * BUFFER_SIZE, '\0', BUFFER_SIZE);
                /* 将收到的数据读到共享内存中 */
                ret = recv(connfd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (res < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    }
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    /* 通过管道将数据发送给主进程 */
                    send(pipefd, (char*)(&idx), sizeof(idx), 0);
                }
            }
            else if (sockfd == pipefd && (events[i].events & EPOLLIN)) {
                int client = 0;
                /* 接收主进程发送过来的数据，即客户数据到达连接的编号 */
                ret = recv(sockfd, (char*)(&client), sizeof(client), 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    }
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    /* 将共享内存中的数据发送出去 */
                    send(connfd, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            } 
            else {
                continue;
            }
        }
    }
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atot(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    user_counter = 0;
    users = new client_data[USER_LIMIT + 1];
    usb_process = new int[PROCESS_LIMIT];
    for (int i = 0; i < PROCESS_LIMIT; ++i) {
        sub_process[i] = -1;
    }

    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, sig_handler);
    bool stop_server = false;
    bool terminate = false;

    /* 创建共享内存 */
    shmfd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    assert(shmfd != -1);
    ret = ftruncate(shmfd, USER_LIMIT * BUFFER_SIZE);
    assert(ret != -1);

    shame_mem = (char*)mmap(NULL, USER_LIMIT * BUFFER_SIZE, 
        PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0); 
    assert(share_mem != MAP_FAILED);
    close(shmfd);

    while(!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)(&client_address), &client_addrlength);

                if (connfd < 0) {
                    printf("errno is %d \n", errno);
                    continue;
                }
                if (user_counter >= USER_LIMIT) {
                    const char *info = "too many users";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    continue;
                }
                users[user_counter].address = client_address;
                users[user_counter].connfd = connfd;
                
                ret = socketpair(PF_UNIX, SOCK_STREAM, 0, users[user_counter].pipefd);
                assert(ret != -1);

                pid_t pid = fork();
                if (pid < 0) {
                    close(connfd);
                    continue;
                }
                else if (pid == 0) { // 子进程
                    close(epollfd);
                    close(listenfd);
                    close(users[user_counter].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_counter, users, share_mem);
                    munmap((void*)share_mem, USER_LIMIT * BUFFER_SIZE);
                    exit(0);
                } 
                else  {
                    close(connfd);
                    close(users[user_counter].pipefd[1]);
                    addfd(epollfd, users[user_counter].pipefd[0]);
                    users[user_counter].pid = pid;
                    sub_process[pid] = user_counter;
                    user_counter++;
                }

            }
            /* 处理信号事件 */ 
            else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (res == -1) {
                    continue;
                }
                else if (ret == 0) {
                    continue;
                }
                else {
                    for (int i = 0; i < ret; ++i) {
                        switch(signals[i]) {
                            /* 子进程退出，说明某个客户端关闭了连接 */
                            case SIGCHLD : {
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1, &sta, WNOHANG)) > 0) {
                                    int del_user = sub_process[pid];
                                    sub_process[pid] = -1;
                                    if (del_user < 0 || del_user > USER_LIMIT) {
                                        continue;
                                    }
                                    epoll_ctl(epollfd, EPOLL_CTL_DEL, users[del_user].pipefd[0]);
                                    close(users[del_user].pipefd[0]);
                                    users[del_user] = users[--user_count];
                                    sub_process[users[del_user].pid] = del_user;
                                }
                                if (terminate && user_counter == 0){
                                    stop_server = true;
                                }
                                break;
                            }
                            case SIGTERM: {

                            }
                            case SIGINT: {
                                /* 结束服务器程序 */
                                printf("kill all the child now\n");
                                if (user_counter == 0) {
                                    stop_server = true;
                                    break;
                                }
                                for (int i = 0; i < user_counter; ++i) {
                                    int pid = users[i].pid;
                                    kill(pid, SIGTERM);
                                }
                                terminate = true;
                                break;
                            }
                            default : break;
                        }
                    }
                }
                /* 子进程向父进程写了数据 */
                else if (events[i].events & EPOLLIN) {
                    int child = 0;
                    ret = recv(sockfd, (char*)(&child), sizeof(child), 0);
                    printf("read data from child accross pipe\n");
                    if (ret == -1) {
                        continue;
                    }
                    else if (ret == 0) {
                        continue;
                    }
                    else {
                        for (int j = 0; j < user_counter; ++j) {
                            if (users[j].pipefd[0] != sockfd) {
                                printf("send data to child accross pipe\n");
                                send(users[j].pipefd[0], (char*)(&child), sizeof(child), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}































