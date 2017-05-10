#ifndef FUNC_H_
#define FUNC_H_

#define BUF_SIZE 500

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

void err_quit(const char* x) 
{ 
    perror(x); 
    exit(1); 
}


#endif /* FUNC_H_ */
