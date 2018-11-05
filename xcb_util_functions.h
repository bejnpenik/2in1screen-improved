#pragma once

#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) > (B) ? (B) : (A))

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

extern xcb_connection_t *conn;
extern xcb_screen_t *scrn;
extern int default_screen_no;
extern xcb_window_t root;


//xcb based functions
int init_xcb (void);
int get_screen(void);
void kill_xcb(void);
char *get_xcb_atom_name(xcb_atom_t);
xcb_atom_t get_atom_by_name(const char *, const size_t);




//randr based functions
int get_crtc_with_name(xcb_randr_crtc_t *, const char *, const size_t );
xcb_randr_screen_size_t *get_screen_size(xcb_window_t);
int set_crtc_config(xcb_randr_crtc_t, uint16_t);
int set_screen_size(uint16_t , uint16_t , uint16_t , uint16_t);
void set_screen_config(uint16_t);

//xinput based functions

xcb_input_device_info_t *find_device_info(const char *, const size_t);
int set_device_property(const char *, const size_t, const char *, const size_t, const char *, const size_t, int, const int, const void *);
int set_device_property_id(xcb_input_device_info_t *, const char *, const size_t, const char *, const size_t, int,  const int, const void *);




