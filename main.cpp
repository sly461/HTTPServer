#include "server/httpserver.h"


int main(int argc, char *argv[]) {

    if (argc <= 1)
    {
        printf("Parameter format：server port\n");
        return 0;
    }
    short port = atoi(argv[1]);
    
    HTTPServer server(port);

    //运行
    server.Run();
    
    return 0;
}