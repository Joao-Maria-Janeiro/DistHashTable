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
#include "server.h"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


int fd,errcode, newfd, client_sockets[3];
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128] = "";
char str[100] = "";


void createServer(Server* server) {
  fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (fd==-1) exit(1); //error

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,server->myPort,&hints,&res);
  if((errcode)!=0)/*error*/exit(1);

  n=bind(fd,res->ai_addr,res->ai_addrlen);
  if(n==-1) /*error*/ exit(1);

  if(listen(fd,5)==-1)/*error*/exit(1);

  while(1) {
    printf("Linha 51\n");

    // Set all sockets to 0
    FD_ZERO(&rfds);

    // Add the main socket to set
    FD_SET(fd, &rfds);
    maxfd = fd;

    FD_SET(STDIN_FILENO  , &rfds);

    // Add all available child sockets to set
    // If there are available sockets
    if(client_sockets[0] > 0) {
      FD_SET(client_sockets[0], &rfds);
    }
    maxfd = max(client_sockets[0], maxfd);

    if(server->prevConnFD > 0) {
      client_sockets[1] = server->prevConnFD;
      FD_SET(server->prevConnFD, &rfds);
    }
    maxfd = max(server->prevConnFD, maxfd);

    if(server->nextConnFD > 0) {
      client_sockets[2] = server->nextConnFD;
      FD_SET(server->nextConnFD, &rfds);
    }
    maxfd = max(server->nextConnFD, maxfd);

    printf("Linha 81\n");
    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <=0) exit(1);
    if(FD_ISSET(STDIN_FILENO , &rfds)) {
      n = read(STDIN_FILENO, buffer, 128);
      if (n > 0) {
        if(strstr(buffer, "leave") != NULL) {
          break;
        } else if(strstr(buffer, "send") != NULL) {
          n = write(server->prevConnFD, "Prev\n", 6);
          n = write(server->nextConnFD, "Next\n", 6);
        }
      }
      continue;
    }
    printf("Linha 96\n");

    if(FD_ISSET(fd, &rfds)) {
      printf("\nGive me some connections baby\n");
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
                 //inform user of socket number - used in send and receive commands  
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newfd , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));   

        if(client_sockets[0] == 0) {
          client_sockets[0] = newfd;
        } else {
          close(newfd);
        }
    }

    //Prevents from reading the same port 2 times (newfd and some prev or next)
    if ((client_sockets[0] == server->nextConnFD) || (client_sockets[0] == server->prevConnFD)) {
        client_sockets[0] = 0;
    }

    //If only 2 servers and prev and next are the same, only read from one of them
    if(server->nextConnFD == server->prevConnFD) {
      client_sockets[2] = 0;
    }

    //Else its on some other socket
    for(int i = 0; i < 3; i++) {
      newfd = client_sockets[i];
      if(FD_ISSET(newfd, &rfds)) {
        if((n = read(newfd, buffer, 128)) == 0) {
          close(newfd);
          printf("Connection closed %d\n\n", i);
          client_sockets[i] = 0;
        } else { 
          buffer[n] = '\0';
          write(1,"received: ",11); write(1,buffer,n); // Print incoming message
          //Now check what the incoming message is asking for
          if(strstr(buffer, "NEW") != NULL) {
            if((server->nextConnFD == 0) && (server->prevConnFD == 0)) { //If there's only 1 server in the ring
              server->prevConnFD = newfd;
              server->doubleNextKey = server->myKey;
              strcpy(server->doubleNextIp, server->myIp);
              strcpy(server->doubleNextPort, server->myPort);

              sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort);
              server->nextConnFD = connectToNextServer(server);
              n = write(server->nextConnFD, "SUCCCONF\n", 10);

              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              n = write(server->prevConnFD, str, strlen(str));
            } else {

            }
          } else if(strstr(buffer, "SUCC ") != NULL) {
              sscanf(buffer, "%s %d %s %s", str, &(server->doubleNextKey), server->doubleNextIp, server->doubleNextPort);
          } else if(strstr(buffer, "SUCCCONF") != NULL) {
              server->prevConnFD = newfd;
          }
        }
      }
    }
    printServerData(server);
  }

for(int i=0; i<3; i++) {
  close(client_sockets[i]);
}

freeaddrinfo(res); 
close(fd);


}

//For handling the entry of a new server to the ring
void serverIsEntering(char buffer[128], int *newfd, Server *server) {
  
}

int connectToNextServer(Server* server) {
  fd = socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (fd == -1) exit(1); //error
  
  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  
  errcode = getaddrinfo(server->nextIp, server->nextPort,&hints,&res);
  if(errcode != 0) {
    printf("GET ADDRESS INFO ERROR\n");
    exit(1);
  }
  
  n = connect(fd,res->ai_addr,res->ai_addrlen);
  if(n == -1) {
    printf("Error code: %d\n", errno);
    printf("CONNECT ERROR\n");
    exit(1);
  }
  return fd;
}

void printServerData(Server* server) {
  printf("\n\n myKey: %d\nnextKey: %d\ndoubleNextKey: %d\nnextConnection: %d\nprevConnection: %d\n\n", server->myKey, server->nextKey, server->doubleNextKey, server->nextConnFD, server->prevConnFD);
}