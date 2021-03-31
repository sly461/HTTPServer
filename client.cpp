#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void sys_err(const char * str) {
    perror(str);
    exit(1);
} 

int main(int argc, char **argv) {
    if(argc <= 2) {
        printf("Parameter format：server IP port\n");
        return 0;
    }
    char * IP = argv[1];
    int port = atoi(argv[2]);
    char buf[BUFSIZ];
    //服务器地址与端口
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    int ret = inet_pton(AF_INET, IP, &serv_addr.sin_addr.s_addr);
    if(ret == -1) sys_err("inet_pton error");
    //构建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1) sys_err("socket error");
    //connect
    ret = connect(clientfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    if(ret == -1) sys_err("connect error");
    //读写数据
    while(1) {
        ret = read(STDIN_FILENO, buf, BUFSIZ);
        ret = write(clientfd, buf, ret);
        if(ret == -1) sys_err("write error");
        ret = read(clientfd, buf, BUFSIZ);
        if(ret == 0) break;
        else if(ret == -1) sys_err("read error");
        //打印到屏幕上
        write(STDOUT_FILENO, buf, ret);
    }
    //关闭socket
    close(clientfd);
    return 0;
}

