#include "csapp.h"

void echo(int connfd){  //11.6.A
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio,connfd);
    while((n=Rio_readlineb(&rio,buf,MAXLINE))!=0){
        printf("server received %d bytes\n",(int)n);
        Rio_writen(connfd,buf,n);
    }
}

int main(int argc,char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc!=2){
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);  //socket하는 과정은 왜 없이 바로 listen? getaddrinfo도없고.. socket bind listen accept 순서.. 함수에 다있음
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
        Getnameinfo((SA *)&clientaddr,clientlen, client_hostname,MAXLINE,client_port,MAXLINE,0);
        printf("Connected to (%s, %s)\n",client_hostname,client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);


}