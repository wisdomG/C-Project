#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

/**************************************
 * 半同步半异步进程池实现
 * ************************************/
#include <sys/types.h>
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
#include <sys/stat.h>

class Process {
public:
    Process() : m_pid(-1) { }
    pid_t m_pid;      // 子进程PID
    int m_pipefd[2];  // 与父进程通信使用的管道
};

template<typename T>
class ProcessPool {
private:
    ProcessPool(int listenfd, int process_number = 8);
public:
    /* 这是一个单例模式 */
    static ProcessPool<T>* create(int listenfd, int process_number = 8) {
        if (!m_instance) { // m_instance = null
            m_instance = new ProcessPool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~ProcessPool() {
        delete[] m_sub_process;
    }
    void run();

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    /* 进程池最多的子进程数 */
    static const int MAX_PROCESS_NUMBER = 16;
    /* 每个进程负责的最大用户数 */
    static const int USER_PER_PROCESS = 65536;
    /* epoll能处理的最多事件数 */
    static const int MAX_EVENT_NUMBER = 10000;
    /* 进程池中的进程总数 */
    int m_process_number;
    /* 子进程在池中的序号 */
    int m_idx;
    /* 每个进程都有一个epoll内核事件表 */
    int m_epollfd;
    /* 监听用socket */
    int m_listenfd;
    /* 子进程通过m_stop判断是否停止执行 */
    int m_stop;
    /* 保存所有子进程的描述信息 */
    Process* m_sub_process;
    /* 进程池实例 */
    static ProcessPool<T>* m_instance;
};

template<typename T>
ProcessPool<T>* ProcessPool<T>::m_instance = NULL;

/* 信号管道，处理信号，实现统一事件源 */
static int sig_pipefd[2];

static int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)(&msg), 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(*handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

/***************************************************************
 * 构造函数
 * 在构造函数中创建若干个子进程，并设置与父进程通信用的管道
 * ************************************************************/
template<typename T>
ProcessPool<T>::ProcessPool(int listenfd, int process_number)
    :m_listenfd(listenfd), m_process_number(process_number), m_idx(-1),
    m_stop(false) {
    assert(process_number > 0 && process_number <= MAX_PROCESS_NUMBER);

    m_sub_process = new Process[process_number];
    assert(m_sub_process);

    for (int i = 0; i < process_number; ++i) {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if (m_sub_process[i].m_pid > 0) { // 父进程
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        } else { // 子进程
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i;
            break; // 这里子进程一定要break，不然也就进行循环往下执行
        }
    }
}

/**************************************************************
 * 统一事件源
 * 将管道描述符加入到epoll事件链表中
 * ***********************************************************/
template<typename T>
void ProcessPool<T>::setup_sig_pipe() {
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    printf("process %d s epollfd %d\n", getpid(), m_epollfd);

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

/**************************************************************
 * 开启父进程与子进程
 * 父进程的m_idx在构造函数中初始化为-1
 * ***********************************************************/
template<typename T>
void ProcessPool<T>::run() {
    if (m_idx != -1) {
        run_child();
        return ;
    }
    run_parent();
}

/*************************************************************
 * 子进程运行
 *
 * **********************************************************/
template<typename T>
void ProcessPool<T>::run_child() {
    setup_sig_pipe();
    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    /* 每个用户就是一个模板类，对应一个连接 */
    T* users = new T[USER_PER_PROCESS];
    assert(users);

    int number = 0;
    int ret = -1;

    while (!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if(sockfd == pipefd && (events[i].events & EPOLLIN)) {
                /* 父进程发过来的消息，让子进程接受一个连接 */
                int client = 0;
                ret = recv(sockfd, (char*)(&client), sizeof(client), 0);
                if ((ret < 0 && errno != EAGAIN) || ret == 0) {
                    continue;
                } else {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr*)(&client_address), &client_addrlength);
                    if (connfd < 0) {
                        printf("errno is %d\n", errno);
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    /* 执行一些初始化动作 */
                    users[connfd].init(m_epollfd, connfd, client_address);
                    printf("Process %d receive a connection at %d fd\n", getpid(), connfd);
                }
            } else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)) {
                /* 子进程接收到的信号 */
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch(signals[i]) {
                            /* 子进程会收到SIGCHLD信号吗 */
                            case SIGCHLD: {
                                printf("child process %d receive SIGCHLD signal\n", getpid());
                                pid_t pid;
                                int stat;
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM: {
                                printf("child process %d receive SIGTERM signal\n", getpid());
                                break;
                            }
                            case SIGINT: {
                                printf("child process %d receive SIGINT signal\n", getpid());
                                m_stop = true;
                                break;
                            }
                            default : {
                                break;
                            }
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                /* 其他可读数据，交个模板T处理即可 */
                users[sockfd].process();
            } else {
                continue;
            }
        }
    }
    delete[] users;
    users = NULL;
    close(pipefd);
    // close(m_listenfd);
    close(m_epollfd);
}

/**************************************************
 * 运行父进程
 *
 * ***********************************************/
template<typename T>
void ProcessPool<T>::run_parent() {
    setup_sig_pipe();   // 该函数对管道好信号统一事件源，并初始化epoll
    addfd(m_epollfd, m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0;
    int ret = -1;

    while (!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                /* 新连接到来 */
                int i = sub_process_counter;
                /* round robin 方式获取一个子进程，m_pid这个参数在构造的时候就已经赋值好了 */
                do {
                    if (m_sub_process[i].m_pid != -1) {
                        break;
                    }
                    i = (i + 1) % m_process_number;
                } while (i != sub_process_counter);
                /* =-1 表示子进程已经退出，说明父进程也该退出了 */
                if (m_sub_process[i].m_pid == -1) {
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i + 1) % m_process_number;
                send(m_sub_process[i].m_pipefd[0], (char*)(&new_conn), sizeof(new_conn), 0);
                printf("send request to child %d at (pid)%d\n", i, m_sub_process[i].m_pid);
            } else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)) {
                /* 处理父进程收到的信号 */
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch(signals[i]) {
                            case SIGCHLD: {
                                printf("parent process receives SIGCHLD signal\n");
                                pid_t pid;
                                int stat;
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    for (int i = 0; i < m_process_number; ++i) {
                                        if (m_sub_process[i].m_pid == pid) {
                                            /* 该子进程结束，关闭信号用管道，并将m_pid置为-1，标记子进程退出 */
                                            printf("child %d join\n", i);
                                            close(m_sub_process[i].m_pipefd[0]);
                                            m_sub_process[i].m_pid = -1;
                                        }
                                    }
                                }
                                /* 所有子进程都退出之后，父进程也退出 */
                                m_stop = true;
                                for (int i = 0; i < m_process_number; ++i) {
                                    if (m_sub_process[i].m_pid != -1) {
                                        m_stop = false;
                                    }
                                }
                                break;
                            }
                            case SIGTERM: {
                                printf("parent process receives SIGTERM signal\n");
                                break;
                            }
                            case SIGINT: {  // 收到ctrl+C
                                printf("parent process receives SIGINT signal\n");
                                printf("kill all the child now\n");
                                for (int i = 0; i < m_process_number; ++i) {
                                    int pid = m_sub_process[i].m_pid;
                                    if (pid != -1) {
                                        kill(pid, SIGTERM);
                                    }
                                }
                                break;
                            }
                            default : {
                                break;
                            }
                        }
                    }
                }
            } else {
                continue;
            }
        }
    }
    close(m_epollfd);
}


#endif

















