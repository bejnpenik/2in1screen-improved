#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
void error(const char *);

int main(int argc, char *argv[])
{
	
	int sockfd, servlen,n;
	struct sockaddr_un  serv_addr;
	char buffer[256];
	int buffer_len = 0;
	char *socket_path = NULL;
	int socket_path_len = 0;
	buffer[0] = '\0';
	if (argc > 1){
	for (int i = 1; i<argc; i++ ){
		if (strncmp(argv[i], "--socket-path", 13) == 0){
			socket_path_len = strlen(argv[i+1]);
			socket_path = (char *)malloc((socket_path_len+ 1)*sizeof(char));
			memcpy(socket_path, argv[++i], socket_path_len);
			socket_path[socket_path_len] = '\0';
			continue;
		}
		strcat(buffer, argv[i]);
		buffer_len += strlen(argv[i]);
		strcat(buffer, " ");
		buffer_len++;
	}
	buffer[--buffer_len] = '\0';
	}	
	bzero((char *)&serv_addr,sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	if (socket_path == NULL){
		strcpy(serv_addr.sun_path,"/tmp/2in1screen.socket");
	}else{
		memcpy(serv_addr.sun_path, socket_path, socket_path_len);
	}
	servlen = strlen(serv_addr.sun_path) + 
		     sizeof(serv_addr.sun_family);
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
	error("Creating socket");
	if (connect(sockfd, (struct sockaddr *) 
		             &serv_addr, servlen) < 0)
	error("Connecting");
	if (send(sockfd,buffer,strlen(buffer), 0) == -1){
	error("Failed to send the data.\n"); 
	}	
	n=read(sockfd,buffer,80);

	close(sockfd);
	return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
