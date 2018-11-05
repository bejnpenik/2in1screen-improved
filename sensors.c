#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#define DATA_SIZE 256
#define N_STATE 4
#define GYRO
char basedir[DATA_SIZE];
char *basedir_end = NULL;
char content[DATA_SIZE];
char *ROT[]   = {"normal", 				"inverted", 			"left", 				"right"};
double accel_y = 0.0,
#if N_STATE == 4
	   accel_x = 0.0,
#endif
	   accel_g = 5.0;
#ifdef GYRO
double anglvel_x = 0.0, anglvel_y = 0.0, anglvel_z = 0.0;
#endif
int current_state = 0;
int run = 0;

int rotation_changed(){
	int state = 0;
	if(accel_y < -accel_g) state = 0;
	else if(accel_y > accel_g) state = 1;
#if N_STATE == 4
	else if(accel_x > accel_g) state = 2;
	else if(accel_x < -accel_g) state = 3;
#endif

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
int main(int argc, char const *argv[]) {
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
#if N_STATE == 4
	FILE *dev_accel_x = bdopen("in_accel_x_raw", 1);
#endif
#ifdef GYRO
	FILE *pfg = popen("ls /sys/bus/iio/devices/iio:device*/in_anglvel*", "r");
	if(!pfg){
		fprintf(stderr, "IO Error.\n");
		return 2;
	}
	memset(basedir, 0, sizeof(basedir));
	if(fgets(basedir, DATA_SIZE , pfg)!=NULL){
		basedir_end = strrchr(basedir, '/');
		if(basedir_end) *basedir_end = '\0';
	}else{
		fprintf(stderr, "Unable to find any gyroscope.\n");
		return 1;
	}
	pclose(pfg);
	bdopen("in_anglvel_scale", 0);
	double gscale = atof(content);
	FILE *dev_anglvel_x = bdopen("in_anglvel_x_raw", 1);
	FILE *dev_anglvel_y = bdopen("in_anglvel_y_raw", 1);
	FILE *dev_anglvel_z = bdopen("in_anglvel_z_raw", 1);
	double vector;
	
#endif
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	run = 1;
	while(run){
		fseek(dev_accel_y, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_y);
		accel_y = atof(content) * scale;
#if N_STATE == 4
		fseek(dev_accel_x, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_x);
		accel_x = atof(content) * scale;
#endif
#ifdef GYRO
		fseek(dev_anglvel_x, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_anglvel_x);
		anglvel_x = atof(content) * gscale;
		fseek(dev_anglvel_y, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_anglvel_y);
		anglvel_y = atof(content) * gscale;
		fseek(dev_anglvel_z, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_anglvel_z);
		anglvel_z = atof(content) * gscale;
		vector = sqrt(anglvel_x*anglvel_x + anglvel_y*anglvel_y+ anglvel_z*anglvel_z);
		while (vector > 0.5){//try to detect moving ... kinda
			continue;
		}
	
#endif
		if(rotation_changed()){
			printf("%s", ROT[current_state]);
			fflush(stdout);
		}
	}
	fclose(dev_accel_y);
#if N_STATE == 4
	fclose(dev_accel_x);
#endif
#ifdef GYRO
	fclose(dev_anglvel_x);
	fclose(dev_anglvel_y);
	fclose(dev_anglvel_z);
#endif
	
	return 0;
}
