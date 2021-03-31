#include <iostream>
#include "config/config.h"

using namespace std;


int main(int argc, char *argv[]) {
    
    Config config;

    HTTPServer server;

    //初始化
    server.init(config.port);

    //监听
    server.eventListen();

    cout << "HTTPServer服务器开启------" << endl;
    //运行
    
    
    return 0;
}