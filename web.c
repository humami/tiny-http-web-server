#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>

void server_static(int connfd, char *filename, int filesize)
{
    int srcfd;
    char *srcp;
    char buf[1024];
    char *filetype = "text/html";

    memset(&buf, 0, sizeof(buf));

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    write(connfd, buf, 1024);

    srcfd = open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    write(connfd, srcp, filesize);
    munmap(srcp, filesize);
}

void parsr_uri(char *uri, char *filename)
{
    strcpy(filename, ".");
    strcat(filename, uri);
    
    if(uri[strlen(uri)-1] == '/')
        strcat(filename, "index.html");
}

void clienterror(int connfd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[1024];
    char body[4096];

    memset(&buf, 0, sizeof(buf));
    memset(&body, 0, sizeof(body));

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em></body></html>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    write(connfd, buf, 1024);
    sprintf(buf, "Content-type: text/html\r\n");
    write(connfd, buf, 1024);
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    write(connfd, buf, 1024);
    write(connfd, body, 4096);
}

void httpserver(int connfd)
{
    struct stat sbuf;
    char buf[1024];
    char method[1024];
    char uri[1024];
    char version[1024];
    char filename[1024];

    memset(&buf, 0, sizeof(buf));
    memset(&method, 0, sizeof(method));
    memset(&uri, 0, sizeof(uri));
    memset(&version, 0, sizeof(version));
    memset(&filename, 0, sizeof(filename));
    
    read(connfd, buf, 1024);

    int i=0;
    int j=0;
    
    //method
    while(buf[i] != ' ')
    {
        method[j] = buf[i];
        
        i++;
        j++;
    }
    i++;
    printf("method: %s\n", method);

    //uri
    j=0;
    while(buf[i] != ' ')
    {
        uri[j] = buf[i];
        
        i++;
        j++;
    }
    i++;
    printf("uri: %s\n", uri);

    //version
    j=0;
    while(buf[i] != '\n')
    {
        version[j] = buf[i];

        i++;
        j++;
    }
    i++;
    printf("version: %s\n", version);

    if(strcasecmp(method, "GET") != 0)
    {
        clienterror(connfd, method, "501", "Not Implemented",
            "Tiny does not implement this method");
        return;
    }

    parsr_uri(uri, filename);    
    printf("%s\n", filename);

    if(stat(filename, &sbuf) < 0)
    {
        clienterror(connfd, filename, "404", "  Not found",
            "Tiny couldn't find this file");
        return;
    }

    server_static(connfd, filename, sbuf.st_size);
}

int main()
{
    int listenfd;
    int connfd;
    int optval = 1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int nready;
    int clientfd[1024];
    fd_set read_set, ready_set;
    int maxfd;
    int maxi;

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

    maxi = -1;
    int i;
    for(i=0; i<1024; i++)
        clientfd[i] = -1;
    maxfd = listenfd;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);

    while(1)
    {
        int client_size = sizeof(client_addr);

        ready_set = read_set;
        nready = select(maxfd+1, &ready_set, NULL, NULL, NULL);

        char buffer[1024];
        memset(&buffer, 0, 1024);

        if(FD_ISSET(listenfd, &ready_set))
        {
            connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_size);
            if(connfd < 0)
                exit(1);

            //add client
            nready--;

            int i;
            for(i=0; i<1024; i++)
            {
                if(clientfd[i] < 0)
                {
                    clientfd[i] = connfd;
                    FD_SET(connfd, &read_set);

                    if(connfd > maxfd)
                        maxfd = connfd;
                    if(i > maxi)
                        maxi = i;
                    break;
                }
            }

            if(i == 1024)
            {
                printf("add client error : Too many clients\n");
                exit(1);
            }
        }

        //check client
        for(i=0; (i<=maxi) && (nready>0); i++)
        {
            connfd = clientfd[i];

            if((connfd > 0) && (FD_ISSET(connfd, &ready_set)))
            {
                nready--;

                read(connfd, buffer, 1024);
                sleep(3);
                write(connfd, buffer, 1024);

                close(connfd);
                FD_CLR(connfd, &read_set);
                clientfd[i] = -1;
            }
        }
    }

    exit(0);
}