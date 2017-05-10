/**
 * This program accepts requests from client and sends messages to the destination
 * Sends collision message when there is a collision.
 */

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
#include <fcntl.h>
#define BUF_SIZE 500
#define LISTENQ 10
#define SLOT_TIME 100
#define MAX_CLIENTS 10

ssize_t writeN(int sockfd,const void *vptr, size_t n);
void *connection_handler(void *socket_desc);
int checkDetails(char *s1, char *s2);
int getSockfd(char *station);
int write_to_log(char *mesg);

char shared_buffer[100] ="\0";
int stationNum =0;
char logfile[] = "logfile_server.txt";
FILE *logfp;
int semaphore=0;

struct client_fds {
	int sockfd;
	char *stationId;
} clients[MAX_CLIENTS];
void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    pid_t childpid;
    struct sockaddr_in serv_addr,cli_addr; 
    socklen_t clilen = sizeof(cli_addr);
    int number_of_clients = 0;
   // logfile = fopen(logfile,"wt");
    // Initialize all the client to -1
    int i;
    for( i=0;i<MAX_CLIENTS;i++)
    {
    	clients[i].sockfd = -1;
    	clients[i].stationId = " ";
    }

    //logfp = fopen("logfile_server.txt", "wt");
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd<0)
        {
            printf("\n Error : Could not create socket on server\n");
            return 1;
        }
    else {
  	  printf("\nSocket created successfully");
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) == 0) {
	printf("\nBind successful %d",listenfd);
    } 
    else {
 	printf("\nBind unsuccessful");
     }

    if(   listen(listenfd, LISTENQ)!=0 ) {
    	printf("\nListen failed");
    } else {
    	printf("\nListen successful");
    }
  //  printf("Waiting for connections");

    int *new_sock;
    while( (number_of_clients<MAX_CLIENTS) && (connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &clilen)) ) {
    		printf("Connection Accepted");
    		add_connection(connfd);
    		number_of_clients++;

    		pthread_t thread_id;
    		new_sock = malloc(sizeof(int));
    		*new_sock = connfd;
    		if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) new_sock) < 0)
    		        {
    		            perror("could not create thread");
    		            return 1;
    		        }

    		        //Now join the thread , so that we dont terminate before the thread
    		        pthread_join( thread_id , NULL);
    		        remove_connection(connfd);
    		        number_of_clients--;
    		        //puts("Handler assigned");
    }
    	if (connfd < 0)
        {
            perror("accept failed");
            return 1;
        }

    return 0;
}

/**
 * Each thread is handled by this method where the actual CBP is implemented
 */
void *connection_handler(void *socket_desc) {
	int sockfd = *(int*)socket_desc;
	    ssize_t read_size;
	    int written;
	    char buf[BUF_SIZE]="\0";
	    char frameNum[20],destNum[20],sourceNum[20];
	    int dest_fd;
	    char collision_mesg[] = "Collision Reported";
	    char data[100]="\0";
	    //Set the socket to Non Blocking mode
	    int rc = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	           if (rc < 0)
	           {
	              perror("fcntl() failed");
	              close(sockfd);
	              exit(-1);
	           } 

	    //Receive a message from client  
	    ssize_t n;
	    while( ( n = read(sockfd, buf, BUF_SIZE) ) )
	    {
	    	if(n<0) {
	    	                if(errno!=EWOULDBLOCK) {
	    	                        perror("  recv() failed");
	    	                        break;
	    	                } 
	    	                else {
	    	                	usleep(SLOT_TIME);
	    	                	continue;
	    	                }
	    	}
	         sscanf(buf,"%s %s %s",frameNum,sourceNum,destNum);				    	// Get source, destination and frame number
	         // Check the station Id and update in client connections if it is not existed
	    	int i;
	         for(i=0;i<MAX_CLIENTS;i++)
	    	{
	    		if(clients[i].sockfd == sockfd) {
	    			clients[i].stationId = strdup(sourceNum);
	    			break;
	    		}
	    	}

	    	if(strlen(shared_buffer)==0)
	    	{
	    		strcpy(shared_buffer,buf);
	    		sprintf(data,"\nReceived Part 1 of %s from %s", frameNum,sourceNum);
	    		printf("\n%s",data);
	    		write_to_log(data);
	    	} else if( checkDetails(shared_buffer,buf) == 1) {
	    		// Send the first part of the message to the destination
	    		if(  getSockfd(destNum)>0)
	    			{
	    			dest_fd = getSockfd(destNum);
	    			written = write(dest_fd , shared_buffer , strlen(shared_buffer));
	    			strcpy(shared_buffer,"\0");
	    			sprintf(data,"\nReceived Part 2 of %s from %s", frameNum,sourceNum);
	    			printf("\n%s",data);
	    			sprintf(data,"\nSent Part 1 of %s from %s to %s", frameNum,sourceNum,destNum);
	    			printf("\n%s",data);
	    			write_to_log(data);
	    				if(strlen(shared_buffer) == 0) {			// Check the shared buffer again
	    					strcpy(shared_buffer,buf);						// Copy the second part to the buffer
	    					write(dest_fd , shared_buffer , strlen(shared_buffer));			// Send the second part of the buffer if the shared buffer is empty
	    					strcpy(shared_buffer,"\0");
	    					sprintf(data,"\nSent Part 2 of %s from %s to %s", frameNum,sourceNum,destNum);
	    					printf("\n%s",data);
	    					write_to_log(data);
	    				}
	    				else {
	    					write(sockfd,collision_mesg,strlen(collision_mesg));
	    					write_to_log(collision_mesg);
	    				}
	    			} else {
	    				sprintf(data,"Destination not found");
	    				printf("\n%s",data);
	    				write_to_log(data);
	    				write(sockfd,data,strlen(data));
	    			}
	    	}
	    	else {
	    		write_to_log(collision_mesg);
	    	    write(sockfd,collision_mesg,strlen(collision_mesg));
	    	}
	        
	    }
	    printf("\ndone");
	    free(socket_desc);
	    return 0;
}

/**
 * This method checks if both the frames are received from same client and are in order
 */
int checkDetails(char *s1, char *s2)
{
	char frame1[20],source1[20],dest1[20];
	char frame2[20],source2[20],dest2[20];

	sscanf(s1,"%s %s %s",frame1,source1,dest1);
	sscanf(s2,"%s %s %s",frame2,source2,dest2);

	if(	(strcmp(frame1,frame2) == 0)
			&& (strcmp(source1,source2)==0)
				&& (strcmp(dest1,dest2) == 0) )
	{
		return 1;
	}
	else
	{
		return -1;
	}

}

/**
 * This method returns a sock id for the given station name only if it is already connected to the server.
 */
int getSockfd(char *station)
{
	int i;
	for( i=0;i<MAX_CLIENTS;i++)
	{
		if(  strcmp(clients[i].stationId,station) == 0)
		{
			return clients[i].sockfd;
		}
	}
	return -1;
}

/**
 * This method adds a client connection to the list of clients connected.
 */
int add_connection(int fd)
{
	int i;
	for( i=0;i<MAX_CLIENTS;i++) {
		if(clients[i].sockfd==-1) {
			clients[i].sockfd = fd;
			return 1;
		}
	}
	return -1;
}

/**
 * This method removes a client connection from the list of clients connected.
 */
int remove_connection(int fd)
{
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		if(clients[i].sockfd==fd) {
			clients[i].sockfd = -1;
			return 1;
		}
	}
	return -1;
}

/**
 * This method writes the given message to the log file
 */
int write_to_log(char *mesg)
{
	if(semaphore==0) {
		semaphore++;
		logfp = fopen(logfile,"a+");
		fprintf(logfp,"%s",mesg);
		  fclose(logfp);
		semaphore--;
		return 0;
	} else {
		sleep(1);
		write_to_log(mesg);
	}
}
