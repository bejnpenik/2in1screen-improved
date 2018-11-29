#pragma once


#define MAX_MSG_LEN 256
#ifndef MAX
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B)         ((A) > (B) ? (B) : (A))
#endif
#define NBR_OF_COMMANDS 7

char socket_path_default[] = "/tmp/2in1screen.socket";
typedef enum {
	LEFT,
	RIGHT, 
	NORMAL,
	INVERTED,
	LOCK,
	UNLOCK,
	TOGGLE,
	BAD_MSG
} Commands;

typedef struct commands_struct{
	int command_len;
	char *command_str;
	Commands command;
} command_t;

command_t commands[NBR_OF_COMMANDS] = {
	{11, "rotate_left", LEFT},
	{12, "rotate_right", RIGHT},
	{13, "rotate_normal", NORMAL},
	{15, "rotate_inverted", INVERTED},
	{13, "lock_rotation", LOCK},
	{15, "unlock_rotation", UNLOCK},
	{11, "toggle_lock", TOGGLE}
};
int handle_msg(char *, size_t);
