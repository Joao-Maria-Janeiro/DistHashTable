#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <sys/select.h>
#include <errno.h>



#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

int fd,errcode, newfd, afd=0;
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

enum {idle, busy} state;

void createServer(char*port) {
  fd=socket(AF_INET,SOCK_STREAM,0);
  if (fd==-1) {
      printf("SOCKET ERROR\n");
      exit(1); //error
  }

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,port,&hints,&res);
    if((errcode)!=0) {
      printf("GETADDRINFO ERROR\n");
      /*error*/exit(1);
    }

  n = bind(fd,res->ai_addr,res->ai_addrlen);
  if(n == -1){
      printf("Error code: %d\n", errno);
      printf("BIND ERROR\n");
       /*error*/ exit(1);
    }

  if(listen(fd,5)==-1)/*error*/exit(1);

  state = idle;

  while(1) {
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    maxfd = fd;

    if(state == busy) {
      FD_SET(afd, &rfds);
      maxfd = max(maxfd, afd);
    }

    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <= 0) exit(1);

    if(FD_ISSET(fd, &rfds)) {
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
      switch(state) {
        case idle:
          afd = newfd;
          state = busy;
          break;
        case busy:
          close(newfd);
          break;
      }
    }

    if(FD_ISSET(afd, &rfds)) {
        if((n = read(afd, buffer, 128)) != 0) {
            if(n==-1) exit(1);
            // Add write function
            n=write(newfd,buffer,n);
            if(n==-1) exit(1);
        } else {
            close(afd);
            state = idle;
        }
    }
  }
  close(fd);
}