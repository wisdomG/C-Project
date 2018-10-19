/** 
 *@desc:  基于reactor模型的服务器程序 
 *@author: zhangyl
 *@date:   2016.11.23
 */

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  //for htonl() and htons()
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>     //for signal()
#include <pthread.h>
#include <semaphore.h>
#include <list>
#include <errno.h>
#include <time.h>
#include <sstream>
#include <iomanip> //for std::setw()/setfill()
#include <stdlib.h>


#define WORKER_THREAD_NUM   5

#define min(a, b) ((a <= b) ? (a) : (b)) 

int g_epollfd = 0;
bool g_bStop = false;
int g_listenfd = 0;
pthread_t g_acceptthreadid = 0;
pthread_t g_threadid[WORKER_THREAD_NUM] = { 0 };
pthread_cond_t g_acceptcond;
pthread_mutex_t g_acceptmutex;

pthread_cond_t g_cond /*= PTHREAD_COND_INITIALIZER*/;
pthread_mutex_t g_mutex /*= PTHREAD_MUTEX_INITIALIZER*/;

pthread_mutex_t g_clientmutex;

std::list<int> g_listClients;

void prog_exit(int signo)
{
    ::signal(SIGINT, SIG_IGN);
    ::signal(SIGKILL, SIG_IGN);
    ::signal(SIGTERM, SIG_IGN);

    std::cout << "program recv signal " << signo << " to exit." << std::endl;

    g_bStop = true;

    ::pthread_cond_broadcast(&g_acceptcond);
    ::pthread_cond_broadcast(&g_cond);

    ::epoll_ctl(g_epollfd, EPOLL_CTL_DEL, g_listenfd, NULL);

    // shutdow可以选择关闭某个方向上的数据输出，通过第二个参数进行控制
    // close实际上是让引用计数-1，只有引用计数为0时，才真正关闭连接
    // 因此，在多进程共享fd时，shutdown会影响所有进程，而close并不会影响其他进程
    ::shutdown(g_listenfd, SHUT_RDWR);
    ::close(g_listenfd);
    ::close(g_epollfd);

    ::pthread_cond_destroy(&g_acceptcond);
    ::pthread_mutex_destroy(&g_acceptmutex);
    
    ::pthread_cond_destroy(&g_cond);
    ::pthread_mutex_destroy(&g_mutex);

    ::pthread_mutex_destroy(&g_clientmutex);
}

// 启动服务器监听，并将监听加入epollfd中进行管理
bool create_server_listener(const char* ip, short port)
{
    g_listenfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (g_listenfd == -1)
        return false;

    int on = 1;
    ::setsockopt(g_listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    ::setsockopt(g_listenfd, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if (::bind(g_listenfd, (sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        return false;

    if (::listen(g_listenfd, 50) == -1)
        return false;

    g_epollfd = ::epoll_create(1);
    if (g_epollfd == -1)
        return false;

    struct epoll_event e;
    memset(&e, 0, sizeof(e));
    // EPOLLRDHUP事件表示对端关闭了连接
    e.events = EPOLLIN | EPOLLRDHUP;
    e.data.fd = g_listenfd;
    // 将listenfd加入epoll中进行监控
    if (::epoll_ctl(g_epollfd, EPOLL_CTL_ADD, g_listenfd, &e) == -1)
        return false;

    return true;
}

// 释放客户端
void release_client(int clientfd)
{
    // 从epoll中删除该fd，然后将其关闭
    if (::epoll_ctl(g_epollfd, EPOLL_CTL_DEL, clientfd, NULL) == -1)
        std::cout << "release client socket failed as call epoll_ctl failed" << std::endl;

    ::close(clientfd);
}

// accept线程，只负责进行accept操作并将其加入到epoll中
void* accept_thread_func(void* arg)
{   
    while (!g_bStop)
    {
        // 先加锁，等待一个条件变量
        ::pthread_mutex_lock(&g_acceptmutex);
        ::pthread_cond_wait(&g_acceptcond, &g_acceptmutex);
        if (g_bStop) {
            printf("Accept thread is ready to exit\n");
            break;
        }
        //::pthread_mutex_lock(&g_acceptmutex);

        //std::cout << "run loop in accept_thread_func" << std::endl;

        struct sockaddr_in clientaddr;
        socklen_t addrlen;
        // 接收一个连接
        int newfd = ::accept(g_listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        ::pthread_mutex_unlock(&g_acceptmutex);
        if (newfd == -1)
            continue;

        std::cout << "new client connected: " << ::inet_ntoa(clientaddr.sin_addr) << ":" << ::ntohs(clientaddr.sin_port) << std::endl;

        // 将新连接设置为非阻塞形式
        int oldflag = ::fcntl(newfd, F_GETFL, 0);
        int newflag = oldflag | O_NONBLOCK;
        if (::fcntl(newfd, F_SETFL, newflag) == -1)
        {
            std::cout << "fcntl error, oldflag =" << oldflag << ", newflag = " << newflag << std::endl;
            continue;
        }

        // 加入到epoll中进行管理
        struct epoll_event e;
        memset(&e, 0, sizeof(e));
        // 设置边沿触发模式
        e.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        e.data.fd = newfd;
        if (::epoll_ctl(g_epollfd, EPOLL_CTL_ADD, newfd, &e) == -1)
        {
            std::cout << "epoll_ctl error, fd =" << newfd << std::endl;
        }
    }

    return NULL;
}

// 工作线程，处理描述符上的读写事件
void* worker_thread_func(void* arg)
{   
    while (!g_bStop)
    {
        // 从队列中取任务，处理接受到的消息
        // 所有客户端共享队列，所以要加锁
        int clientfd;
        ::pthread_mutex_lock(&g_clientmutex);
        // 防止虚假唤醒，这里使用while循环，而不是if判断
        while (g_listClients.empty()) {
            ::pthread_cond_wait(&g_cond, &g_clientmutex);
            if (g_bStop) {
                printf("Worker thread is ready to exit\n");
                pthread_mutex_unlock(&g_clientmutex);
                return NULL;
            }
        }

        clientfd = g_listClients.front();
        g_listClients.pop_front();  
        pthread_mutex_unlock(&g_clientmutex);

        // 加上这一行是为了gdb调试时能够实时刷新输出
        std::cout << std::endl;

        std::string strclientmsg;
        char buff[10]; // 这个为了观察现象，将buff设置的小一点
        bool bError = false;
        // 因为是边沿触发模式的非阻塞IO，所以要使用循环将缓冲区中的数据读完
        while (true)
        {
            memset(buff, 0, sizeof(buff));
            // 接收消息
            int nRecv = ::recv(clientfd, buff, 10, 0);
            if (nRecv == -1)
            {
                // EWOULDBLOCK 说明缓冲区的数据已经被我们读完了
                // 则退出循环处理本次数据
                if (errno == EWOULDBLOCK)
                    break;
                else
                {
                    std::cout << "recv error, client disconnected, fd = " << clientfd << std::endl;
                    release_client(clientfd);
                    bError = true;
                    break;
                }
                    
            }
            // 客户端关闭连接
            else if (nRecv == 0)
            {
                std::cout << "peer closed, client disconnected, fd = " << clientfd << std::endl;
                release_client(clientfd);
                bError = true;
                break;
            }
            // nRecv > 0的情况
            strclientmsg += buff;
            printf("read %d data\n", nRecv);
        }

        // 出错就不需要往下继续执行了
        if (bError) {
            printf("bError == true\n");
            continue;
        }
        
        std::cout << "client msg: " << strclientmsg << std::endl;

        // 将收到的消息加上时间戳进行返回
        time_t now = time(NULL);
        struct tm* nowstr = localtime(&now);
        std::ostringstream ostimestr;
        ostimestr << "[" << nowstr->tm_year + 1900 << "-" 
                  << std::setw(2) << std::setfill('0') << nowstr->tm_mon + 1 << "-" 
                  << std::setw(2) << std::setfill('0') << nowstr->tm_mday << " "
                  << std::setw(2) << std::setfill('0') << nowstr->tm_hour << ":" 
                  << std::setw(2) << std::setfill('0') << nowstr->tm_min << ":" 
                  << std::setw(2) << std::setfill('0') << nowstr->tm_sec << "]server reply: ";

        strclientmsg.insert(0, ostimestr.str());
        
        // 循环发送数据
        while (true)
        {
            int nSent = ::send(clientfd, strclientmsg.c_str(), strclientmsg.length(), 0);
            if (nSent == -1)
            {
                // EWOULDBLOCK说明发送缓冲区已经满了，因此只能等待下次发送了
                if (errno == EWOULDBLOCK)
                {
                    ::sleep(10);
                    continue;
                }
                else
                {
                    std::cout << "send error, fd = " << clientfd << std::endl;
                    release_client(clientfd);
                    break;
                }
                   
            }          

            std::cout << "send: " << strclientmsg;
            strclientmsg.erase(0, nSent);

            if (strclientmsg.empty()) {
                std::cout << std::endl;
                break;
            }
        }
    }

    return NULL;
}

// 以守护进程方式运行
void daemon_run()
{
    int pid;
    signal(SIGCHLD, SIG_IGN);
    //1）在父进程中，fork返回新创建子进程的进程ID；  
    //2）在子进程中，fork返回0；  
    //3）如果出现错误，fork返回一个负值；  
    pid = fork();
    if (pid < 0)
    {
        std:: cout << "fork error" << std::endl;
        exit(-1);
    }
    //父进程退出，子进程独立运行  
    else if (pid > 0) {
        exit(0);
    }
    //之前parent和child运行在同一个session里,parent是会话（session）的领头进程,  
    //parent进程作为会话的领头进程，如果exit结束执行的话，那么子进程会成为孤儿进程，并被init收养。  
    //执行setsid()之后,child将重新获得一个新的会话(session)id。  
    //这时parent退出之后,将不会影响到child了。  
    setsid();
    int fd;
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
    }
    if (fd > 2)
        close(fd);
 
}


int main(int argc, char* argv[])
{  
    short port = 0;
    int ch;
    bool bdaemon = false;
    // -d 以守护进行方式运行 -p port 指定端口号
    while ((ch = getopt(argc, argv, "p:d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            bdaemon = true;
            break;
        case 'p':
            port = atol(optarg);
            break;
        }
    }

    if (bdaemon)
        daemon_run();

    if (port == 0)
        port = 12345;
     
    if (!create_server_listener("0.0.0.0", port))
    {
        std::cout << "Unable to create listen server: ip=0.0.0.0, port=" << port << "." << std::endl;
        return -1;
    }
    
    // 设置信号处理
    signal(SIGCHLD, SIG_DFL); // 默认信号处理
    signal(SIGPIPE, SIG_IGN); // 忽略管道信号
    signal(SIGINT, prog_exit);
    signal(SIGKILL, prog_exit);
    signal(SIGTERM, prog_exit);

    ::pthread_cond_init(&g_acceptcond, NULL);
    ::pthread_mutex_init(&g_acceptmutex, NULL);

    ::pthread_cond_init(&g_cond, NULL);
    ::pthread_mutex_init(&g_mutex, NULL);

    ::pthread_mutex_init(&g_clientmutex, NULL);
     
    // 这里还新建了一个主线程，用于接收连接
    ::pthread_create(&g_acceptthreadid, NULL, accept_thread_func, NULL);
    
    // 建立线程池并启动工作线程
    for (int i = 0; i < WORKER_THREAD_NUM; ++i)
    {
        ::pthread_create(&g_threadid[i], NULL, worker_thread_func, NULL);
    }

    while (!g_bStop)
    {       
        struct epoll_event ev[1024];
        int n = ::epoll_wait(g_epollfd, ev, 1024, 10);
        if (n == 0)
            continue;
        else if (n < 0)
        {
            std::cout << "epoll_wait error" << std::endl;
            continue;
        }

        int m = min(n, 1024);
        for (int i = 0; i < m; ++i)
        {
            // 通知正在等待的线程处理新连接
            if (ev[i].data.fd == g_listenfd)
                pthread_cond_signal(&g_acceptcond);
            else
            {               
                // 通知普通工作线程处理接收数据
                pthread_mutex_lock(&g_clientmutex);              
                g_listClients.push_back(ev[i].data.fd);
                pthread_mutex_unlock(&g_clientmutex);
                pthread_cond_signal(&g_cond);
                //std::cout << "signal" << std::endl;
            }
                
        }

    }
    
    return 0;
}
