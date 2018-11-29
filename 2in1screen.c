#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xinput.h>
#include "xcb_util_functions.h"
#include "ipc.h"

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
static int sock_fd, cli_fd, read_len;
static int run = 0;
static int only_touchinput = 0;
static int only_crtc = 0;
static int crtc_disconnected = 0;
int current_state = 0;
pid_t bar;
char *bar_command_path = NULL;
char *socket_path = NULL;
int db;
int sel_fd;
static char buffer[MAX_MSG_LEN] = {0};
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



void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}
void repaint_bar(void){
	if (bar_command_path != NULL){
		kill(bar, 15);
		if (current_state == 2 || current_state==3){
				if (socket_path != NULL){
					char *bar_comm[] = {bar_command_path, "--socket-path", socket_path, "--orientation", "vertical", NULL};
					bar = popen2(bar_comm, NULL, NULL);
				}else{
					char *bar_comm[] = {bar_command_path, "--orientation", "vertical", NULL};
					bar = popen2(bar_comm, NULL, NULL);
				}
		}else{
			if (socket_path != NULL){
					char *bar_comm[] = {bar_command_path, "--socket-path", socket_path, "--orientation", "horizontal", NULL};
					bar = popen2(bar_comm, NULL, NULL);
				}else{
					char *bar_comm[] = {bar_command_path, "--orientation", "horizontal", NULL};
					bar = popen2(bar_comm, NULL, NULL);
				}
		}
	}
	
}
void exec_command(int command){
	switch (command){
		case LEFT:
			current_state = 2;
			if (!lock_rotation && !only_touchinput){
				if (only_crtc) rotate_only_crtc();
				else rotate();
				repaint_bar();
			}else
			if (only_touchinput){
				rotate_only_touchinput();
			}
			break;
		case RIGHT:
			current_state = 3;
			if (!lock_rotation && !only_touchinput){
				if (only_crtc) rotate_only_crtc();
				else rotate();
				repaint_bar();
			}else
			if (only_touchinput){
				rotate_only_touchinput();
			}
			break;
		case NORMAL:
			current_state = 0;
			if (!lock_rotation && !only_touchinput){
				if (only_crtc) rotate_only_crtc();
				else rotate();
				repaint_bar();
			}else
			if (only_touchinput){
				rotate_only_touchinput();
			}
			break;
		case INVERTED:
			current_state = 1;
			if (!lock_rotation && !only_touchinput){
				if (only_crtc) rotate_only_crtc();
				else rotate();
				repaint_bar();
			}else
			if (only_touchinput){
				rotate_only_touchinput();
			}
			break;
		case LOCK:
			lock_rotation = 1;
			break;
		case UNLOCK:
			lock_rotation = 0;
			break;
		case TOGGLE:
			lock_rotation = !lock_rotation;
			break;
	}
}

int main(int argc, char const *argv[]) {
	/// G E T    A R G U M E N T S ///
	int Error = 0;
	bar_command_path = NULL;
	socket_path = NULL;
	if (argc > 1){
		for (int i = 1; i < argc; i++){
			if (strncmp(argv[i], "--rotate-only-touchinput", 24)==0){	
				only_touchinput = 1;	
			}else
			if (strncmp(argv[i], "--rotate-only-output", 20)==0){	
				only_crtc = 1;	
			}else
			if (strncmp(argv[i], "--socket-path", 13) == 0){
				int path_len = strlen(argv[i+1]);
				socket_path = (char *)malloc((path_len+ 1)*sizeof(char));
				memcpy(socket_path, argv[i+1], path_len);
				socket_path[path_len] = '\0';
			}else
			if (strncmp(argv[i], "--bar-command-path", 18)==0){
				
				int path_len = strlen(argv[i+1]);
				bar_command_path = (char *)malloc((path_len+ 1)*sizeof(char));
				memcpy(bar_command_path, argv[i+1], path_len);
				bar_command_path[path_len] = '\0';
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
	///   //////   ///
	/// I N I T   B A R ///
	
	if (bar_command_path != NULL){
		if (socket_path != NULL){
			char *bar_comm[] = {bar_command_path, "--socket-path", socket_path, NULL};
			bar = popen2(bar_comm, NULL, NULL);
		}else{
			char *bar_comm[] = {bar_command_path, NULL};
			bar = popen2(bar_comm, NULL, NULL);
		}
	}
	///  //////  ///
	/// X C B   F I L E   D E S C R I P T O R   ///
	int conn_fd = xcb_get_file_descriptor(conn);
	///  //////  ///
	/// A D D   R A N D R   N O T I F Y   E V E N T   ///
	xcb_randr_select_input(conn, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  	xcb_flush(conn);
	xcb_randr_screen_change_notify_event_t* randr_evt;
	xcb_generic_event_t* evt;
  	xcb_timestamp_t last_time;
	///   //////   ///
	///   E S T A B L I S H   S E R V E R   ///
	struct sockaddr_un  sock_adress;
	if ((sock_fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0){
		Error = 1;
		goto cleanup;
	}
	sock_adress.sun_family = AF_UNIX;
	if (socket_path == NULL){
		strcpy(sock_adress.sun_path, socket_path_default);
	}else{
		strcpy(sock_adress.sun_path, socket_path);
	}
	unlink(sock_adress.sun_path);
	if (bind(sock_fd, (struct sockaddr *) &sock_adress, sizeof(sock_adress)) == -1) {
		Error = 1;
		goto cleanup;
	}
	if (listen(sock_fd, SOMAXCONN) == -1) {
		Error = 1;
		goto cleanup;	
	}
	int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	///   //////   ///
	/// R E G I S T E R   S I G N A L S ///
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	///  //////  ///
	/// S T A R T ///
	run = 1;
	char buff[256] = {0};
	fd_set descriptors;
	sel_fd = MAX(sock_fd, conn_fd) + 1;
    last_time = XCB_CURRENT_TIME;
	while(run){
		FD_ZERO(&descriptors);
		FD_SET(conn_fd, &descriptors);
		FD_SET(sock_fd, &descriptors);
		sel_fd = MAX(sock_fd, conn_fd) + 1;
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(conn_fd, &descriptors)){
				while ((evt = xcb_poll_for_event(conn)) != NULL) {
					if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
						randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
			    		if (last_time != randr_evt->timestamp) {
							last_time = randr_evt->timestamp;
							if (monitor_with_name_disconnected(CRTC_NAME, CRTC_NAME_LEN) != 0){
								printf("Crtc disconnected!\n");
								crtc_disconnected = 1;
								only_touchinput = 1;
							}else
							if (crtc_disconnected){
								printf("Crtc reconnected!\n");
								crtc_disconnected = 0;
								only_touchinput = 0;
								if (!get_crtc_with_name(&crtc, CRTC_NAME, CRTC_NAME_LEN) && !only_touchinput){
									fprintf(stderr, "Failed to find crtc with name %s", CRTC_NAME);
									Error = 1;
									goto cleanup;
								}
							}
			    		}
				    }
					free(evt);
				}
				
			}
			if (FD_ISSET(sock_fd, &descriptors)){
				cli_fd = accept(sock_fd, NULL, 0);
				if (cli_fd < 0 || (read_len = recv(cli_fd, buff, sizeof(buff)-1, 0)) < 0){ 
					fprintf(stderr, "Error connecting");
					continue;
}
				int ret = handle_msg(buff, read_len);
				if (ret != BAD_MSG){
					write(cli_fd,"STATUS OK",9);
					exec_command(ret);
				}else{
					 write(cli_fd,"STATUS BAD",10);
				}
				
   				close(cli_fd);
				
			}
		}
		if (xcb_connection_has_error(conn)){
			run = 0;
		}	
	}
	/// C L E A N U P ///
cleanup:
	if (socket_path) free(socket_path);
	if (bar_command_path) free(bar_command_path);
	if (device_info) free(device_info);
	free(scrn_size);
	kill_xcb();
	return Error;
}

int handle_msg(char *msg, size_t msg_len){
	int i = 0;
	for (i = 0; i < NBR_OF_COMMANDS; i++){
		if (strncmp(msg, commands[i].command_str, commands[i].command_len) == 0){
			return commands[i].command;
		}
	}
	return BAD_MSG;
}

