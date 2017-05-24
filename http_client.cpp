#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <iostream>

#define BUFSIZE 1024

int write_n_bytes(int fd, char * buf, int count);

int main(int argc, char * argv[]) {
    char * server_name = NULL;
    int server_port = 0;
    char * server_path = NULL;

    int sock = 0;
    int rc = -1;
    int datalen = 0;
    bool ok = true;
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * host = NULL;
    char * req = NULL;

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;
    
    struct timeval timeout;
    fd_set set;

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];



    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }

    /* create socket */
	sock = minet_socket(SOCK_STREAM);
	if (sock == -1)
		return sock;
    // Do DNS lookup
    /* Hint: use gethostbyname() */
	host = gethostbyname(server_name);
	if(host == NULL){
		close(sock);
		return -1;
	}
    /* set address */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(server_port);
	sa.sin_addr.s_addr = *( unsigned long *) host->h_addr_list[0];
	if (connect(sock, (struct sockaddr *) &sa, sizeof(sa)) != 0){
		close(sock);
		printf("fail to connect\n");
		return -1;
	}
 
 char request[100];
sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", server_path, server_name);
    /* send request */
	FD_SET(sock, &set);
	minet_write(sock, request, strlen(request));
    /* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */   
if ((minet_select(sock+1, &set, NULL, NULL, NULL)) == -1)
{
	fprintf(stderr, "Select Error\n");
}
 
int numBytesRead;
char* endHeaderPtr;
char* header;
char workingBuf[50*BUFSIZE];

char restbuff[BUFSIZE];
memset(&workingBuf, 0, 50*BUFSIZE);
header = &workingBuf[0];
   /* first read loop -- read headers */
	while(1){
		numBytesRead = minet_read(sock, buf, BUFSIZE);
		//std::cout << numBytesRead << std::endl;
//		printf(buf); fflush(stdout);
		if (numBytesRead == 0){
			printf("no bytes left to read\n");
			break;
		}
		buf[numBytesRead] = '\0';
		if ((endHeaderPtr = strstr(buf, "\r\n\r\n"))){
			endHeaderPtr = strstr(buf, "\r\n\r\n");
			
			int pos = endHeaderPtr - buf;
		//	std::cout << pos << std::endl;
			char subbuff[BUFSIZE];
			strcpy(subbuff, buf);
			subbuff[pos] = '\0';
			int restLength = strlen(buf) - pos;
			memcpy(restbuff, &subbuff[pos+1], restLength);
			restbuff[restLength] = '\0';
		//printf(subbuff);fflush(stdout);
			//printf("test\n");
			strcpy(header, subbuff);
			header = header + strlen(subbuff);
			break;
		}

		else{
			strcpy(header, &buf[0]);
			header = header + strlen(buf);
		}
		


	memset(&buf, 0, BUFSIZE);
		
	}
/*
for (int i=0; workingBuf[i] != '\0'; i++){
	printf("%c", workingBuf[i]);
	fflush(stdout);
}  

printf("HI\n");
*/
 
    /* examine return code */   
    //Skip "HTTP/1.0"
    //remove the '\0'
    // Normal reply has return code 200
    //
char httpv[10];
char status[10];
sscanf( workingBuf, "%s %s", httpv, status);
//printf(status); fflush(stdout);
    /* print first part of response */
	if ( strcmp(status, "200") == 0){
		
		fprintf(stdout, restbuff);
		memset(workingBuf, 0, 50*BUFSIZE);
		header = &workingBuf[0];
		ok = true;
		while(1){
			numBytesRead = minet_read(sock, buf, BUFSIZE);
			printf(buf); fflush(stdout);
			if (numBytesRead == 0){
				printf("End of File\n");
				break;
			}	
			strcpy(header, &buf[0]);
			fprintf(stdout, buf);
			header = header+strlen(buf);
		}
	}else{
		fprintf(stderr, workingBuf);	
		ok = false;
	}
    /* second read loop -- print out the rest of the response */
    



    /*close socket and deinitialize */
	minet_close(sock);
	minet_deinit();

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
	totalwritten += rc;
    }
    
    if (rc < 0) {
	return -1;
    } else {
	return totalwritten;
    }
}


