#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


int main()
{
    int listenfd;
    int clientfd;
    int optval = 1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8888);

    if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < -1)
        return -1;

    if(listen(listenfd, 1) < 0)
        return -1;

    int client_size = sizeof(client_addr);
    char buffer[1024];

    while(1)
    {
        clientfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_size);

        if(clientfd < 0)
            exit(1);

        memset(&buffer, 0, sizeof(buffer));

        sleep(3);
        read(clientfd, buffer, 1024);
        write(clientfd, buffer, 1024);

        close(clientfd);
    }
    
    exit(0);
}