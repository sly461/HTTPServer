/**
 * 基于epoll模型实现http-server
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_EVENTS 5

//三个回调函数声明
void acceptConn(int , int , void *);
void recvData(int , int , void *);
void sendData(int , int , void *);


struct myevent_s {
    int fd;                                             //要监听的文件描述符
    int events;                                          //对应的监听事件
    void *arg;                                          //参数
    void (*call_back)(int fd, int events, void *arg);    //回调函数
    int status;                                         //是否挂在红黑树上监听
    char buf[BUFSIZ*1000];
    int len;
    long last_active;                                   //记录每次加入红黑树的时间值
};

struct myevent_s g_events[MAX_EVENTS+1];
int g_efd;


void perr_exit(const char * err) {
    perror(err);
    exit(1);
}

//初始化事件结构体
void eventset(struct myevent_s * ev, int fd, void (*call_back)(int, int, void*), void *arg) {
    ev->fd = fd;
    ev->events = 0;
    ev->arg = arg;
    ev->call_back = call_back;
    ev->status = 0;
    //bzero(ev->buf, sizeof(ev->buf));
    //ev->len = 0;
    ev->last_active = time(NULL);       //调用eventset函数的时间
}

//添加事件到红黑树上
void eventadd(int efd, int events, struct myevent_s *ev) {
    struct epoll_event epv;
    int op;
    epv.events = ev->events = events;
    epv.data.ptr = ev;
    
    //修改状态
    if(ev->status == 0) {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if(epoll_ctl(efd, op, ev->fd, &epv) < 0)
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
    else printf("event add OK [fd=%d], op=%d, events[%0X]\n", ev->fd, op, events);
}

void eventdel(int efd, struct myevent_s * ev) {
    struct epoll_event epv;
    if(ev->status != 1) return;
    
    ev->status = 0;
    epv.data.ptr = NULL;
    epv.events = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

// 获取一行 \r\n 结尾的数据 

int get_line(int cfd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size-1) && (c != '\n')) {  
        n = recv(cfd, &c, 1, 0);
        if (n > 0) {     
            if (c == '\r') {            
                n = recv(cfd, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n')) {              
                    recv(cfd, &c, 1, 0);
                } else {                       
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        } else {      
            c = '\n';
        }
    }
    buf[i] = '\0';
    
    if (-1 == n)
    	i = n;

    return i;
}

//状态行：状态码、状态名称  消息报头：内容类型、内容长度、connection
void setRespondHeader(struct myevent_s * ev, int status, const char *desp, 
                      const char *type, long len, const char * connection) {
    //状态行
    sprintf(ev->buf+strlen(ev->buf), "HTTP/1.1 %d %s\r\n", status, desp);
    //消息报头
    sprintf(ev->buf+strlen(ev->buf), "Content-Type:%s\r\n", type);
    sprintf(ev->buf+strlen(ev->buf), "Content-Length:%ld\r\n", len);
    sprintf(ev->buf+strlen(ev->buf), "Connection:%s\r\n", connection);
    //空行是必须的 即使响应正文为空
    sprintf(ev->buf+strlen(ev->buf), "\r\n");
    
    ev->len = strlen(ev->buf);
}
//构造错误页面
void setError(struct myevent_s * ev, int status, const char * desp, const char *text) {
    sprintf(ev->buf+strlen(ev->buf), "<!DOCTYPE html>\n<html><head>\n");
    sprintf(ev->buf+strlen(ev->buf), "<title>%d %s</title>\n", status, desp);
    sprintf(ev->buf+strlen(ev->buf), "</head><body>\n<h1>%s</h1>\n", desp);
    sprintf(ev->buf+strlen(ev->buf), "<p>%s</p>\n<hr>\n", text);
    sprintf(ev->buf+strlen(ev->buf), "<address>HTTP/1.1 (Ubuntu) Server.</address>\n</body></html>");

    ev->len = strlen(ev->buf);
}
//文件
void setFile(struct myevent_s * ev, const char* filename) {
    //打开文件
    int fd = open(filename, O_RDONLY);
    if(fd == -1) {
        perr_exit("open error");
    }
    
    //循环读文件 默认ev->buf够大
    int len = read(fd, ev->buf+ev->len, sizeof(ev->buf)-ev->len);
    if(len == -1) perr_exit("read file error");
    // 此处不能直接ev->len = strlen(ev->buf); 
    //可能某个文件的字节流中某个字节8位全为0 解析为字符则为'\0'
    ev->len += len;
}

//目录
void setDir(struct myevent_s * ev, const char* dirname) {
    sprintf(ev->buf+strlen(ev->buf), "<!DOCTYPE html><html>\n");
    sprintf(ev->buf+strlen(ev->buf), "<head><title>文件服务器</title></head>\n");

    //拼一个html页面 <table></table>
    sprintf(ev->buf+strlen(ev->buf), "<body><h1>当前目录: %s</h1><table>\n", dirname);
    //表头
    sprintf(ev->buf+strlen(ev->buf), "<tr><th>Name</th><th>Size</th></tr>\n");
    //目录项二级指针
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);

    char filepath[1024] = {0};
    //遍历
    for(int i=0; i<num; i++) {
        const char * name = ptr[i]->d_name;
        
        //拼接文件的完整路径
        sprintf(filepath, "%s%s", dirname, name);

        //获取文件信息
        struct stat st;
        stat(filepath, &st);
        
        //文件
        if(S_ISREG(st.st_mode)) {
            sprintf(ev->buf+strlen(ev->buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>\n",
                    name, name, (long)st.st_size);
        }
        //目录
        else if(S_ISDIR(st.st_mode)) {
            sprintf(ev->buf+strlen(ev->buf),
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>\n",
                    name, name, (long)st.st_size);
        }

    }

    sprintf(ev->buf+strlen(ev->buf), "</table></body></html>");
    ev->len = strlen(ev->buf);
    
}

const char * getFileType(const char * name) {
    //自左向右查找字符'.'  若不存在返回NULL
    const char * dot = strrchr(name, '.');
    //纯文本格式
    if(dot == NULL)
        return "text/plain; charset=utf-8";
    if(strcmp(dot, ".html")==0 || strcmp(dot, ".htm")==0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void http_request(struct myevent_s * ev) {
    //拆分请求行
    char method[12], path[1024], protocol[12];
    sscanf(ev->buf, "%[^ ] %[^ ] %[^ ]", method, path, protocol);

    //清空缓冲区
    bzero(ev->buf, sizeof(ev->buf));
    ev->len = 0;

    //去掉path中的/ 获取访问文件名
    const char * file = path+1;
    
    //特殊情况 未指定访问资源 默认显示资源目录中内容
    if(strcmp(path, "/") == 0) {
        file = "./";
    }

    //stat获取文件属性
    struct stat st;
    int ret = stat(file, &st);


    //响应行 响应头 响应体
    //404
    if(ret == -1) {
        setRespondHeader(ev, 404, "Not Found", getFileType(".html"), -1, "close");
        setError(ev, 404, "Not Found", "No such file or directory.");
    }
    //文件
    else if(S_ISREG(st.st_mode)) {
        setRespondHeader(ev, 200, "OK", getFileType(file), st.st_size, "close");
        setFile(ev, file);
    }
    //目录
    else {
        setRespondHeader(ev, 200, "OK", getFileType(".html"), -1, "close");
        setDir(ev, file);
    }
      
}

void recvData(int connectfd, int events, void *arg)
{
    //客户端有数据就绪
    struct myevent_s * ev  = (struct myevent_s *) arg;

    //请求行
    int len = get_line(connectfd, ev->buf, BUFSIZ);
    ev->len = len;

    if(len == 0) {
        printf("客户端断开连接...\n");
        /* ev-g_events 地址相减得到偏移元素位置 */  
	    printf("[fd=%d] pos[%ld], closed\n", connectfd, ev-g_events);
        eventdel(g_efd, ev);
        close(connectfd);  
    }
    else if(len > 0){
        printf("============= 请求头 ============\n");   
        printf("请求行数据: %s", ev->buf);
        // 还有数据未读完，将其读走
        while(1) {
            char line[BUFSIZ] = {0};
            len = get_line(connectfd, line, BUFSIZ);
            if(line[0] == '\n') break;
            else if(len == -1) break;
            //printf("请求头数据: %s", line);
        }
        printf("============= The End ============\n");
        //处理请求行"GET / HTTP/1.1"
        //判断GET请求
        if(strncasecmp("get", ev->buf, 3) == 0) {
            //处理http请求
            http_request(ev);
        }
        

        //将该socketfd从树上摘下
        eventdel(g_efd, ev);
        //重新挂上写事件
        eventset(ev, connectfd, sendData, ev);
        eventadd(g_efd, EPOLLOUT | EPOLLET, ev);
    }
    else {
        //错误处理
        printf("recv[fd=%d] error[%d]:%s\n", connectfd, errno, strerror(errno));
        eventdel(g_efd, ev);
        close(connectfd);
    }

}

void sendData(int connectfd, int events, void *arg) {
    struct myevent_s * ev  = (struct myevent_s *) arg;
    int ret;
    int bytes_left = ev->len;
    int written_bytes = 0;
    while(bytes_left > 0) {
        ret = send(connectfd, ev->buf+written_bytes, bytes_left, 0);
        if(ret == 0) continue;
        if(ret < 0) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("se error:");
                continue;
            } else {
                perr_exit("send error");
            }
        }
        written_bytes += ret;
        bytes_left -= ret;
        printf("%d %d\n", written_bytes, bytes_left);
    }

    //将该socketfd从树上摘下
    eventdel(g_efd, ev);
    
    if (ret > 0)
    {
        printf("send[fd=%d], [%d]%s\n", connectfd, ret, ev->buf);
        //不用挂上读事件了 直接关闭socket
        //清空缓存区
        bzero(ev->buf, sizeof(ev->buf));
        ev->len = 0;
        eventset(ev, connectfd, NULL, ev);
        //eventadd(g_efd, EPOLLIN, ev);
        close(connectfd);
        printf("[fd=%d] pos[%ld], closed\n", connectfd, ev-g_events);  
    }
    else
    {
        //错误处理
        close(connectfd);
        printf("send[fd=%d] error[%d]:%s\n", connectfd, errno, strerror(errno));
    }
}

//listenfd对应的回调函数
void acceptConn(int listenfd, int events, void * arg) {
    //客户端地址
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int i;
    int connectfd;
    if ((connectfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {  
	    if (errno != EAGAIN && errno != EINTR) {  
	        /* 暂时不做出错处理 */  
	    }  
	    printf("%s: accept, %s\n", __func__, strerror(errno));
        return ;
	} 
    
    do {
        //找到一个空闲元素
        for(i=0; i<MAX_EVENTS; i++) {
            if(g_events[i].status == 0) break;
        }
        //结构体数组装不下了
        if(i == MAX_EVENTS) {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);  
            break;
        }

        //设置为非阻塞
        int flag = fcntl(connectfd, F_GETFL);
        flag |= O_NONBLOCK;
        fcntl(connectfd, F_SETFL, flag);

        //初始化对应结构体并添加到红黑树上
        eventset(&g_events[i], connectfd, recvData, &g_events[i]);
        eventadd(g_efd, EPOLLIN | EPOLLET, &g_events[i]);

    }while(0);
    
    printf("new connect [%s:%d][time:%ld], pos[%d]\n",
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), g_events[i].last_active, i);  

}

//初始化listenfd
void initListenfd(int efd, short port) {
    //创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //设置非阻塞
    // int flag = fcntl(listenfd, F_GETFL);
    // flag |= O_NONBLOCK;
    // fcntl(listenfd, F_SETFL, flag);
    //端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    //服务器地址与端口
    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind绑定
    bind(listenfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    //设置监听上限
    listen(listenfd, 128);
    //初始化对应的事件结构体 一般listenfd放到结构体数组的末尾
    eventset(&g_events[MAX_EVENTS], listenfd, acceptConn, &g_events[MAX_EVENTS]);
    //添加到红黑树上
    eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);
}

void epollRun(short port) {

    g_efd = epoll_create(MAX_EVENTS+1);
    if(g_efd <= 0) 
        printf("create efd in %s err %s\n", __func__, strerror(errno));

    //初始化listenfd
    initListenfd(g_efd, port);

    //保存已经满足的就绪事件
    struct epoll_event events[MAX_EVENTS+1];

   
    while (1)
    {
        //不用超市验证 http本来就是连接上 请求数据返回数据 然后就断开连接
        // /* 超时验证，每次测试100个链接，不测试listenfd 当客户端60秒内没有和服务器通信，则关闭此客户端链接 */
        // int check = 0;
        // long now = time(NULL);
        // for(int i=0; i<100; i++) {
        //     if(check == MAX_EVENTS) check=0;
        //     //未在树上
        //     if(g_events[check].status == 0) continue;
        //     //是否超时
        //     if(now-g_events[check].last_active < 60) continue;
        //     //关闭socket 从树上del
        //     close(g_events[check].fd);
        //     eventdel(g_efd, &g_events[check]);
        //     printf("[fd=%d] timeout\n", g_events[check].fd);
        // }
        /*监听红黑树g_efd, 将满足的事件的文件描述符加至events数组中, 1秒没有事件满足, 返回 0*/ 
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);
        
        if(nfd < 0) {
            printf("epoll_wait error, exit\n");
            break;
        }
        //遍历
        for(int i=0; i<nfd; i++) {
            struct myevent_s * ev = (struct myevent_s *)events[i].data.ptr;
            
            //读就绪事件
            if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            //写就绪事件
            if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }

    //退出前 释放所有资源
}

//未写错误处理
int main(int argc, char **argv)
{
    // if (argc <= 2)
    // {
    //     printf("Parameter format：server port dir\n");
    //     return 0;
    // }
    // short port = atoi(argv[1]);

    //改变进程的工作目录
    int ret = chdir("/home/ubuntu/test_linux/2.network_programming/9.http_server/");
    if(ret == -1) perr_exit("chdir error");
    //epoll模型跑起来 开始监听
    epollRun(5555);
    
    return 0;
}