#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define DATA_SIZE 256
#define WAIT 1
char basedir[DATA_SIZE];
char *basedir_end = NULL;
char content[DATA_SIZE];
char *socket_path = NULL;
char *ROT_CMD[]   = {
	"rotate_normal", 				
	"rotate_inverted", 			
	"rotate_left", 				
	"rotate_right"
};
double accel_y = 0.0,
	   accel_x = 0.0,
	   accel_g = 5.0;

int current_state = 0;
int run = 0;

int rotation_changed(){
	int state = 0;
	if(accel_y < -accel_g) state = 0;
	else if(accel_y > accel_g) state = 1;
	else if(accel_x > accel_g) state = 2;
	else if(accel_x < -accel_g) state = 3;


	if(current_state!=state){
		current_state = state;
		return 1;
	}
	else return 0;
}

FILE* bdopen(char const *fname, char leave_open){
	*basedir_end = '/';
	strcpy(basedir_end+1, fname);
	FILE *fin = fopen(basedir, "r");
	setvbuf(fin, NULL, _IONBF, 0);
	fgets(content, DATA_SIZE, fin);
	*basedir_end = '\0';
	if(leave_open==0){
		fclose(fin);
		return NULL;
	}
	else return fin;
}

void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}
int send_over_socket(char *socket_name, size_t socket_name_len){
	int sockfd, servlen,n;
	struct sockaddr_un  serv_addr;
	serv_addr.sun_family = AF_UNIX;
	if (socket_name == NULL){
		strcpy(serv_addr.sun_path,"/tmp/2in1screen.socket");
	}else{
		memcpy(serv_addr.sun_path,socket_name, socket_name_len);
	}
	servlen = strlen(serv_addr.sun_path) + 
		         sizeof(serv_addr.sun_family);
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
		fprintf(stderr,"Creating socket");
	if (connect(sockfd, (struct sockaddr *) 
		             &serv_addr, servlen) < 0)
		fprintf(stderr, "Connecting");
	if (send(sockfd,ROT_CMD[current_state],strlen(ROT_CMD[current_state]), 0) == -1){
		fprintf(stderr, "Failed to send the data.\n"); 
	}
	char buffer[256] = {0};
	n=read(sockfd,buffer,80);
	write(1,buffer,n);
	close(sockfd);	
}


int main(int argc, char const *argv[]) {
	int path_len = 0;
	if (argc > 1){
		for (int i = 0; i < argc; i++){
			if (strncmp(argv[i], "--socket-path", 13) == 0){
				path_len = strlen(argv[i+1]);
				socket_path = (char *)malloc((path_len+ 1)*sizeof(char));
				memcpy(socket_path, argv[i+1], path_len);
				socket_path[path_len] = '\0';
			}
		}
	}
	FILE *pf = popen("ls /sys/bus/iio/devices/iio:device*/in_accel*", "r");
	if(!pf){
		fprintf(stderr, "IO Error.\n");
		return 2;
	}

	if(fgets(basedir, DATA_SIZE , pf)!=NULL){
		basedir_end = strrchr(basedir, '/');
		if(basedir_end) *basedir_end = '\0';
	}
	else{
		fprintf(stderr, "Unable to find any accelerometer.\n");
		return 1;
	}
	pclose(pf);

	bdopen("in_accel_scale", 0);
	double scale = atof(content);

	FILE *dev_accel_y = bdopen("in_accel_y_raw", 1);
	FILE *dev_accel_x = bdopen("in_accel_x_raw", 1);
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	run = 1;
	while(run){
		fseek(dev_accel_y, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_y);
		accel_y = atof(content) * scale;
		fseek(dev_accel_x, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_x);
		accel_x = atof(content) * scale;
		if(rotation_changed()){
			send_over_socket(socket_path, path_len);
		}
		sleep(WAIT);
	}
	fclose(dev_accel_y);
	fclose(dev_accel_x);
	if (socket_path != NULL) free(socket_path);
	return 0;
}
