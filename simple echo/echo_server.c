#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#define BUF_SIZE 1200
#define LISTENQ 10

void echo_server(int fd);
ssize_t writeN(int sockfd,const void *vptr, size_t n);

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in serv_addr,cli_addr; 
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd<0)
        {
            printf("\n Error : Could not create socket on server\n");
            return 1;
        }
    else {
  	  printf("\nSocket created successfully");
    }
    //bzero(&serv_addr,sizeof(serv_addr));
    //bzero(&cli_addr,sizeof(cli_addr));

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) == 0) {
	printf("\nBind successful");
    } 
    else {
 	printf("\nBind unsuccessful");
     }

    if(listen(listenfd, LISTENQ)!=0) {
	printf("\nListen failed");
    } else {
	printf("\nListen successful");
    }

    while(1) {
	clilen = sizeof(cli_addr);
	connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &clilen); 
	if (connfd < 0) {
      		printf("\nAccept failed: %s", strerror(errno));
    	}
    	else{
      		printf("\nAccept succeeded: %d", connfd);
    	}
	if((childpid = fork())==0 ) {
		close(listenfd);
		echo_server(connfd);
		exit(0);
	}
    }

        close(connfd);
        sleep(1);
}

void echo_server(int fd) {
	ssize_t n;
	char buf[BUF_SIZE];
	again:
	while ( ( n = read (fd, buf, BUF_SIZE) ) > 0 ) {	// Read the client input
		writeN(fd, buf, n);				// Write the output to the client
		if (n < 0 && errno == EINTR)
			goto again;
		else if (n < 0)
			err_sys("echo_server: read error");
		}	
}

ssize_t writeN(int sockfd,const void *vptr, size_t n) {
	ssize_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0) {
		if( (nwritten = write(sockfd,vptr,nleft)) <=0) {
			if(errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
	nleft-= nwritten;
	ptr+= nwritten;
	}
	return n;
}


