#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xinput.h>
#include "xcb_util_functions.h"

/// D E F I N I T I O N S///
const char CRTC_NAME[] = "eDP1";
const int CRTC_NAME_LEN = 4;
const char DEVICE_NAME[] = "ELAN2514:00 04F3:262F";
const int DEVICE_NAME_LEN = 21;
const char PROPERTY_NAME[] = "Coordinate Transformation Matrix";
const int PROPERTY_NAME_LEN = 32;
const char TYPE_NAME[] = "FLOAT";
const int TYPE_NAME_LEN = 5;
const int COORD_NBR = 9;
const int FORMAT = 32;
static float COORD_NORMAL[] = {1, 0, 0, 0, 1, 0, 0, 0, 1}; 
static float COORD_INVERTED[] = {-1, 0, 1, 0, -1, 1, 0, 0, 1}; 
static float COORD_LEFT[] = {0, -1, 1, 1, 0, 0, 0, 0, 1}; 
static float COORD_RIGHT[] = {0, 1, 0, -1, 0, 1, 0, 0, 1};
static int lock_rotation = 0;
xcb_connection_t *conn;
xcb_screen_t     *scrn;
xcb_randr_crtc_t crtc;
xcb_input_device_id_t device_id;
xcb_input_device_info_t *device_info = NULL;
xcb_randr_screen_size_t *scrn_size;
int default_screen_no;
xcb_window_t root;

static int run = 0;
static int only_touchinput = 0;
static int only_crtc = 0;
int current_state = 0;
pid_t bar;
int db;
int sel_fd;

/// E N D   D E F I N I T I O N S///


pid_t popen2(char **command, int *infp, int *outfp)
{
	int READ = 0, WRITE = 1;
	int p_stdin[2], p_stdout[2];
	pid_t pid;

	if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
		return -1;

	pid = fork();

	if (pid < 0)
		return pid;
	else if (pid == 0)
	{
		close(p_stdin[1]);
		dup2(p_stdin[READ], READ);
		close(p_stdout[READ]);
		dup2(p_stdout[WRITE], WRITE);

		execv(*command, command);
		perror("execvp");
		exit(1);
	}

	if (infp == NULL)
		close(p_stdin[WRITE]);
	else
		*infp = p_stdin[WRITE];

	if (outfp == NULL)
		close(p_stdout[READ]);
	else
		*outfp = p_stdout[READ];

	return pid;
}

void rotate(void){
	switch(current_state){
		case 0:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_0);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_0);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->width, scrn_size->height, scrn_size->mwidth, scrn_size->mheight);
			set_device_property(DEVICE_NAME, DEVICE_NAME_LEN, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_NORMAL);
			break;
		case 1:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_180);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_180);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->width, scrn_size->height, scrn_size->mwidth, scrn_size->mheight);
			set_device_property(DEVICE_NAME, DEVICE_NAME_LEN, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_INVERTED);
			break;
		case 2:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_90);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_90);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->height, scrn_size->width, scrn_size->mheight, scrn_size->mwidth);
			set_device_property(DEVICE_NAME, DEVICE_NAME_LEN, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_LEFT);
			break;
		case 3:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_270);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_270);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->height, scrn_size->width, scrn_size->mheight, scrn_size->mwidth);
			set_device_property(DEVICE_NAME, DEVICE_NAME_LEN, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_RIGHT);
			break;
		default:
			break;
	}
}
void rotate_only_crtc(void){
	switch(current_state){
		case 0:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_0);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_0);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->width, scrn_size->height, scrn_size->mwidth, scrn_size->mheight);
			break;
		case 1:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_180);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_180);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->width, scrn_size->height, scrn_size->mwidth, scrn_size->mheight);
			break;
		case 2:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_90);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_90);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->height, scrn_size->width, scrn_size->mheight, scrn_size->mwidth);
			break;
		case 3:
			set_crtc_config(crtc, XCB_RANDR_ROTATION_ROTATE_270);
			set_screen_config(XCB_RANDR_ROTATION_ROTATE_270);
			xcb_randr_set_screen_size(conn, scrn->root, scrn_size->height, scrn_size->width, scrn_size->mheight, scrn_size->mwidth);
			break;
		default:
			break;
	}
}
void rotate_only_touchinput(void){
	switch(current_state){
		case 0:
			set_device_property_id(device_info, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_NORMAL);
			break;
		case 1:
			set_device_property_id(device_info, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_INVERTED);
			break;
		case 2:
			set_device_property_id(device_info, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_LEFT);
			break;
		case 3:
			set_device_property_id(device_info, PROPERTY_NAME, PROPERTY_NAME_LEN, TYPE_NAME, TYPE_NAME_LEN, FORMAT, COORD_NBR, COORD_RIGHT);
			break;
		default:
			break;
	}
}


void repaint_bar(char *bar_command_path, int sensors_fd){
	kill(bar, 15);
	if (current_state == 2 || current_state==3){
		char *bar_comm[] = {bar_command_path, "vertical", NULL};
		bar = popen2(bar_comm, NULL, &db);
	}else{
		char *bar_comm[] = {bar_command_path, "horizontal", NULL};
		bar = popen2(bar_comm, NULL, &db);
	}
	fcntl(db, F_SETFL, O_RDWR | O_NONBLOCK);
	sel_fd = MAX(sensors_fd, db) + 1;
	
}


void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}
int main(int argc, char const *argv[]) {
	/// G E T    A R G U M E N T S ///
	int Error = 0;
	char *bar_command_path = NULL;
	if (argc > 1){
		for (int i = 1; i < argc; i++){
			if (strncmp(argv[i], "--rotate-only-touchinput", 24)==0){	
				only_touchinput = 1;	
			}else
			if (strncmp(argv[i], "--bar-command-path", 18)==0){
				
				int path_len = strlen(argv[i+1]);
				bar_command_path = (char *)malloc((path_len+ 1)*sizeof(char));
				memcpy(bar_command_path, argv[i+1], path_len);
				bar_command_path[path_len] = '\0';
				
			}else
			if (strncmp(argv[i], "--rotate-only-output", 20)==0){	
				only_crtc = 1;	
			}
		}
	}
	///  //////  ///
	if (only_crtc && only_touchinput){
		fprintf(stderr, "Decisions, decisions...\n");
		Error = 1;
		goto cleanup;
	}
	/// I N I T    X C B ///
	init_xcb();
	get_screen();
	if (!get_crtc_with_name(&crtc, CRTC_NAME, CRTC_NAME_LEN) && !only_touchinput){
		fprintf(stderr, "Failed to find crtc with name %s", CRTC_NAME);
		Error = 1;
		goto cleanup;
	}
	if ((device_info = find_device_info(DEVICE_NAME, DEVICE_NAME_LEN)) == NULL){
		fprintf(stderr, "Failed to find device with name %s", DEVICE_NAME);
		Error = 1;
		goto cleanup;
	}
	root = scrn->root;
	scrn_size = get_screen_size(scrn->root);
	
	/// I N I T   B A R ///
	char *bar_comm[] = {bar_command_path, NULL};
	if (bar_command_path != NULL){
		bar = popen2(bar_comm, NULL, &db);
		fcntl(db, F_SETFL, O_RDWR | O_NONBLOCK);
	}else{
		db = -1;
	}
	///  //////  ///
	/// I N I T    S E N S O R S ///
	char cwd[256];
	if (getcwd(cwd, sizeof(cwd)) == NULL) exit(1);
	char sensor_exec_path[256] = {0};
	sensor_exec_path[0] = '\0';
	strcat(sensor_exec_path, cwd);
	strcat(sensor_exec_path, "/sensors");
	char *sensor_comm[] = {sensor_exec_path, NULL};
	int sensors_fd; pid_t sensors = popen2(sensor_comm, NULL, &sensors_fd);
	fcntl(sensors_fd, F_SETFL, O_RDWR | O_NONBLOCK);
	///  //////  ///
	/// R E G I S T E R   S I G N A L S ///
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	///  //////  ///
	/// S T A R T ///
	run = 1;
	char buff[256] = {0};
	fd_set descriptors;
	sel_fd = MAX(db, sensors_fd) + 1;
	while(run){
		FD_ZERO(&descriptors);
		FD_SET(db, &descriptors);
		FD_SET(sensors_fd, &descriptors);
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(db, &descriptors)){
				memset(buff, 0, 256);
				int size = read(db, buff, 256);
				if (size > 0 && !(size == -1 && errno == EAGAIN)){
					if (strcmp(buff, "LOCKED")==0) lock_rotation = 1;
					else if (strcmp(buff, "UNLOCKED")==0) lock_rotation = 0;
					if (!lock_rotation && !only_touchinput){
						if (only_crtc) rotate_only_crtc();
						else rotate();
						repaint_bar(bar_command_path, sensors_fd);
					}else
					if (only_touchinput){
						rotate_only_touchinput();
					}
				}
			}
			if (FD_ISSET(sensors_fd, &descriptors)){
				memset(buff, 0, 256);
				int size = read(sensors_fd, buff, 256);
				if (size > 0 && !(size == -1 && errno == EAGAIN)){
					if (strcmp(buff, "normal")==0) current_state = 0;
					else if (strcmp(buff, "inverted")==0) current_state = 1;
					else if (strcmp(buff, "left")==0) current_state = 2;
					else if (strcmp(buff, "right")==0) current_state = 3;
					else continue;
					if (!lock_rotation && !only_touchinput){
						if (only_crtc) rotate_only_crtc();
						else rotate();
						repaint_bar(bar_command_path, sensors_fd);
					}else
					if (only_touchinput){
						rotate_only_touchinput();
					}
					
				}
			}
		}
		if (xcb_connection_has_error(conn)){
			run = 0;
		}	
	}
	/// C L E A N U P ///
cleanup:
	if (bar_command_path) free(bar_command_path);
	if (device_info) free(device_info);
	free(scrn_size);
	kill_xcb();
	return Error;
}
