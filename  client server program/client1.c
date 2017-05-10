/**
 * This program accepts input from a file and fragments each line into two mesgs
 * Sends each message to the server
 * Waits for random time specified by Binary back off algorithm it there is any collision and then transmits again
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include<math.h>
#include "func.h"
#define BUF_SIZE 500
#define true 1
#define false 1

void send_frame(FILE *fp,int sockfd, FILE *logfp, char  *stationId);
int binExpBackOff(int collisionNum);
void fragment(char *sendBuff,char *mesg1, char *mesg2, char *stationId, char *frameNum, char *destNum);

int main(int argc, char *argv[])
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    FILE *fp,*logfp;                                 /*file descriptors for input file and log file*/
    char logfile[25];
    char *stationId;

    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server>",argv[0]);
        return 1;
    }
    stationId = strdup(argv[1]);

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

    if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0)
    {
        printf("\ninet_pton error occurred");
        return 1;
    } else {
	printf("\ninet_pton successful");
	printf("\n%s",argv[2]);
	}

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0)
    {
       printf("\nError : Connect Failed");
       return 1;
    } else {
	printf("\nConnection Successful");
    }
    
     int rc = fcntl(sockfd, F_SETFL, O_NONBLOCK);
       if (rc < 0)
       {
          perror("fcntl() failed");
          close(sockfd);
          exit(-1);
       }
       strcpy(logfile,stationId);
       strcat(logfile,"_logfile.txt");
     fp = fopen("sample.txt","r");		/* READS the data from sample.txt file*/
     logfp = fopen(logfile, "wt");			/*writes to Logfile for output*/
     send_frame(fp,sockfd,logfp,stationId);

    return 0;
}

/**
 * This function sends the frames to the server
 */
void send_frame(FILE *fp,int sockfd, FILE *logfp, char  *stationId) {
	char sendBuff[BUF_SIZE],recvBuff[BUF_SIZE];
    int collisionNum = 0;
	char mesg1[BUF_SIZE],mesg2[BUF_SIZE];
    long slotTime = 100;
    char frameNum[20], destNum[20];
    char data[100];

	//Read the file until the end of the file is encountered
	while((fgets(sendBuff,BUF_SIZE,fp))!=NULL) {

        //Fragment the frame to 2 parts
        fragment(sendBuff,mesg1,mesg2,stationId,frameNum,destNum);				// Fragments the frame into two

		do {
            writeN(sockfd,mesg1,strlen(mesg1)); // Send the data to server
            sprintf(data,"Sending %s Part 1 to %s", frameNum,destNum);
            fprintf(logfp,"\n%s",data);
            usleep(slotTime);

            int rc = readline(sockfd,recvBuff,BUF_SIZE);
            if(rc<0) {
                if(errno!=EWOULDBLOCK) {
                        perror("  recv() failed");
                        sprintf(data,"%s","Error receiving data from the server");
                        fprintf(logfp,"\n%s",data);
                        break;
                }
                else {
            	// Send second part
            	                      writeN(sockfd,mesg2,strlen(mesg2));
            	                      sprintf(data,"Sending %s Part 2 to %s", frameNum,destNum);
            	                      fprintf(logfp,"\n%s",data);
            	                      usleep(slotTime);
            	                      break;
                }
            }
            else  if(strstr(recvBuff,"collision")!=NULL) {
                printf("/nCollsion Reported");
                sprintf(data,"%s", "Collision Reported");
                fprintf(logfp,"\n%s",data);
                collisionNum++;
                int slots  = binExpBackOff(collisionNum);					// Calls binaryExpbackoff method to return a random number based on collision attempts
                if(slots<0) {
                	 sprintf(data,"%s", "Attempt limit exceeded");
                	 fprintf(logfp,"\n%s",data);
                		err_quit("Attempt limit exceeded");
                }
                long usec = slots * slotTime;
                usleep(usec);
                continue;
             } else {
            	 printf("\nReceived from Server: %s",recvBuff);
            	sprintf(data,"Received from Server \n%s", recvBuff);
            	fprintf(logfp,"\n%s",data);
             }
        } while(true);
    }
	fclose(logfp);	// Close the files
	fclose(fp);

}

/**
 * This method fragments a frame into two messages
 */
void fragment(char *sendBuff,char *mesg1, char *mesg2, char *stationId, char *frameNum, char *destNum) {
		//char *frameNum;
		strcpy(frameNum,strtok(sendBuff, " "));
		char *token;
		//char *destNum;
		char *tokenArray[100];
		//char *mesg1 = (char *)malloc(sizeof(char)*100);
		//char *mesg2 = (char *)malloc(sizeof(char)*100);
		int i = 0;

		if(frameNum!=NULL) {
			token = strtok(NULL," ");
			strcpy(destNum,token);

			while(token!=NULL) {
				token = strtok(NULL," ");
				if(token!=NULL) {
					tokenArray[i]=strdup(token);
					i++;
				}
			}

			strcpy(mesg1,frameNum);
			strcpy(mesg2,frameNum);
			strcat(mesg1," ");
			strcat(mesg2, " ");
			strcat(mesg1, stationId);
			strcat(mesg2,stationId);
			strcat(mesg1," ");
			strcat(mesg2, " ");
			strcat(mesg1,destNum);
			strcat(mesg2,destNum);
			int j;
			for(j=0;j<(i/2);j++) {
				strcat(mesg1," ");
				strcat(mesg1,tokenArray[j]);
			}
			for(j=(i/2);j<i;j++) {
				strcat(mesg2," ");
				strcat(mesg2,tokenArray[j]);
			}
		}

		//printf("\nMesg 1: %s",mesg1);
		//printf("\nMesg 2: %s",mesg2);
}

/**
 * This method returns a random number based on the number of collisions occured
 */
int binExpBackOff(int collisionNum) {
    int maxValue;
    int minValue;
    int random;
    if(collisionNum<10) {
        maxValue =((int) pow(2.0,(double)collisionNum))-1;
        minValue = 0;
        random = rand()%maxValue + minValue;
    } else if(collisionNum>10 && collisionNum<16) {
        maxValue = 1023;
        minValue = 0;
        random = rand()%maxValue + minValue;
    } else {
        random = -1;
    }
    return random;
}
