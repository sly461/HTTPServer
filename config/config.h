/**
 * 记录各种配置信息
**/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "../httpserver.h"

class Config
{
public:
    Config();
    ~Config();

    //端口
    int port;

    //listenfd触发模式
    int listenTrigMode;

    //connfd触发模式
    int connTrigMode;    

};


#endif