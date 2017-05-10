#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#define BUF_SIZE 1200

void echo_client(FILE *fp, int sockfd);
ssize_t writeN(int sockfd,const void *vptr, size_t n);
ssize_t readfunc(int fd, char *ptr);
ssize_t readline(int fd, void *vptr, size_t maxlen);
void err_quit(const char* x);

void err_quit(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

void echo_client(FILE *fp,int sockfd) {
	char sendBuff[BUF_SIZE],recvBuff[BUF_SIZE];
	FILE *recvfile = fopen("Received.txt","wt");
	int count =0;
	char printCount[20];
	printf("\n\nResponse from Server:");
	//Read the file until the end of the file is encountered
	while((fgets(sendBuff,BUF_SIZE,fp))!=NULL) {
		writeN(sockfd,sendBuff,strlen(sendBuff)); // Send the data to server

		if(readline(sockfd,recvBuff,BUF_SIZE) == 0) {			// Read the data from the server
			err_quit("\necho_client: server terminated prematurely");
		}
		
		sprintf (printCount, "Read Call #: %d", ++count);
		fputs(printCount,recvfile);	// writes to a file
		fputs(recvBuff,recvfile);
		printf("\n");
		fputs(printCount,stdout);	//writes to the standard output
		printf("\n");
		fputs(recvBuff,stdout);
		if(strcmp(sendBuff,recvBuff)==0) {	// Compare the sent data and received data
			fputs("Data Matched\n",stdout);
		} else {
			fputs("Data mismatch\n",stdout);
		}
		
	}
	fclose(recvfile);	// Close the files
	fclose(fp);

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

ssize_t readfunc(int fd, char *ptr) {
	static int read_cnt = 0;
	static char *read_ptr;
	static char read_buf[BUF_SIZE];
	if (read_cnt <= 0) {
	again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
 	}
	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	int n, rc;
	char c, *ptr;
	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = readfunc(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break; 
		} else if (rc == 0) {
			*ptr = 0;
			return(n-1); 
		} else	
			return(-1); 
	}
	*ptr = 0; 
	return(n);
 }

int main(int argc, char *argv[])
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server>",argv[0]);
        return 1;
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError : Could not create socket");
        return 1;
    } else {
	printf("\nSocket created");
	}

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\ninet_pton error occurred");
        return 1;
    } else {
	printf("\ninet_pton successful");
	}

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0)
    {
       printf("\nError : Connect Failed");
       return 1;
    } else {
	printf("\nConnection Successful");
    }
 
     FILE *fp = fopen("sample.txt","r");
     echo_client(fp,sockfd);
	
    
    return 0;
}


