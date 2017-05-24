#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <iostream>

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in sa,sa2;
  int rc,i;
  fd_set readlist;
  fd_set connections;
  int maxfd;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }
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
   
 /* set address */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(server_port);
	sa.sin_addr.s_addr = INADDR_ANY;

  /* bind listening socket */
	if (minet_bind(sock,  &sa) == -1){
		printf("ERROR\n");
	}


  /* start listening */
	if(minet_listen(sock, 5) == -1){
		printf("Listening error\n");
}

FD_ZERO(&connections);
FD_ZERO(&readlist);

FD_SET(sock, &connections);
maxfd = sock;

  /* connection handling loop */
  while(1)
  {
    /* create read list */
	readlist = connections;
    /* do a select */
	if(minet_select(maxfd+1, &readlist, NULL, NULL, NULL) == -1){
		printf("Error in selecting\n");
	}
    /* process sockets that are ready */
	for (i=0; i < maxfd + 1; i++){
	//	std::cout<< i << std::endl;
		if (FD_ISSET(i, &readlist)){
			if(i == sock){
				if ((sock2 = minet_accept(sock, &sa2)) == -1){
					printf("Error in accepting..\n");
				}
				else{
					FD_SET(sock2, &connections);
	//				std::cout<< sock2 << std::endl;
					if (sock2 > maxfd){
						maxfd = sock2;
					}
				}
			}else{
//				printf("before handling..\n");	
				rc = handle_connection(i);			
				FD_CLR(i, &connections);
			}
	
		}

	}


  }
}

int handle_connection(int sock2)
{
  char filename[FILENAMESIZE+1];
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE];
  char *header;
  char *endheaders;
  char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"\
                         "</body></html>\n";
  bool ok=true;


int numBytesRead;
char* endHeaderPtr;
//char* header;
char  workingBuf[BUFSIZE];
//memset(&workingBuf, 0, 25*BUFSIZE);
header = &workingBuf[0];
//printf("handling\n");
//std::cout << &ok_response << std::endl;
  /* first read loop -- get request and headers*/
while(1){

	if((endHeaderPtr = strstr(workingBuf, "\r\n\r\n"))){
//		printf("End of Header\n");
		break;
	}

	numBytesRead = minet_read(sock2, buf, BUFSIZE);
//	std::cout<<numBytesRead<<std::endl;
//	printf(buf);fflush(stdout);
	if (numBytesRead == 0){
//		printf("no more bytes left\n");
		break;
	}
	buf[numBytesRead] = '\0';
	if ((endHeaderPtr = strstr(buf, "\r\n\r\n"))){
		int pos = endHeaderPtr - buf;
		char subbuff[BUFSIZE];
		strcpy(subbuff, buf);
		subbuff[pos] = '\0';
		strcpy(header, &subbuff[0]);
		header += strlen(subbuff);
		break;
	}	
	else{
		strcpy(header, &buf[0]);
		header += strlen(buf);;	
	}
	memset(&buf, 0, BUFSIZE);

	

}
/*
for (int i=0; workingBuf[i] != '\0'; i++){
	printf("%c", workingBuf[i]);
	fflush(stdout);
} */ 
char method[50];
sscanf(workingBuf, "%s %s", method, filename);
//printf(filename);fflush(stdout);
memset(buf, 0, BUFSIZE);
if((fd= open(filename, O_RDONLY)) >= 0){
//	printf("FILE EXISTS\n");
	minet_write(sock2, ok_response_f, strlen(ok_response_f));
	while((rc = read(fd, buf, BUFSIZE))  != 0){
		buf[strlen(buf)] = '\0';
		minet_write(sock2, buf, BUFSIZE);
		memset(&buf, 0, BUFSIZE);
	}
	
}
else{
//	printf("File does not exist\n");
	minet_write(sock2, notok_response, strlen(notok_response));
}

memset(&workingBuf, 0, BUFSIZE);
minet_close(sock2);

  if (ok)
    return 0;
  else
    return -1;
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}

