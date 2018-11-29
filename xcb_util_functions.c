#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xinput.h>
#include "xcb_util_functions.h"

int init_xcb (void){
	conn = xcb_connect(NULL, &default_screen_no);
	if (xcb_connection_has_error(conn)){
		fprintf(stderr, "unable connect to the X server");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
void kill_xcb (void){
	if (*conn)
		xcb_disconnect(*conn);
}


int get_screen(void){
	scrn = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	if (*scrn == NULL){
		fprintf(stderr, "unable to retrieve screen informations");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

char *get_xcb_atom_name(xcb_atom_t atom){
	xcb_generic_error_t *e = NULL;
	xcb_get_atom_name_cookie_t atom_name_cookie = xcb_get_atom_name(conn, atom);
	xcb_get_atom_name_reply_t *atom_name_reply = xcb_get_atom_name_reply(conn, atom_name_cookie, &e);
	if (e == NULL){
		fprintf(stderr, "unable to query atom name");
		return NULL;
	}
	char *atom_name = xcb_get_atom_name_name(atom_name_reply);
	int atom_name_len = xcb_get_atom_name_name_length(atom_name_reply);
	char *tmp = NULL;
	tmp = (char*)malloc((atom_name_len+1)*sizeof(char));
	if (tmp != NULL){
		fprintf(stderr, "unable to malloc");
		free(atom_name_reply);
		return NULL;
	}
	memcpy(tmp, atom_name, atom_name_len);
	tmp[atom_name_len] = '\0';
	free(atom_name_reply);
	return tmp;
}

xcb_atom_t get_atom_by_name(const char *atom_name, const size_t atom_name_len){
	xcb_generic_error_t *e = NULL;
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, atom_name_len, atom_name);
	xcb_intern_atom_reply_t *reply =  xcb_intern_atom_reply (conn, cookie, &e);	
	if (e != NULL){
		fprintf(stderr, "error query atom by name");
		free(reply);
		return -1;
	}
	xcb_atom_t atom = reply->atom;
	free(reply);
	return atom;
}


int get_crtc_with_name(xcb_connection_t *conn, xcb_screen_t *scrn, xcb_randr_crtc_t *crtc, const char *name, const size_t name_len){
	int len;
	int Found = 0;
	xcb_randr_get_screen_resources_reply_t *sres = NULL;
	xcb_randr_get_output_info_cookie_t *cookies = NULL;
	xcb_randr_get_output_info_reply_t **info = NULL;
	xcb_randr_get_crtc_info_reply_t *cir = NULL;
	sres = xcb_randr_get_screen_resources_reply(conn, xcb_randr_get_screen_resources(conn, scrn->root), NULL);
	if (sres == NULL) {
		goto E0;
	}
	len = xcb_randr_get_screen_resources_outputs_length(sres);
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(sres);
	
	cookies = (xcb_randr_get_output_info_cookie_t *) malloc(len*sizeof(xcb_randr_get_output_info_cookie_t));
	if (cookies == NULL){
		goto E0;
	}
	for (int i = 0; i < len; i++) {
		cookies[i] = xcb_randr_get_output_info(conn, outputs[i], XCB_CURRENT_TIME);
	}
	
	info = (xcb_randr_get_output_info_reply_t **)malloc(len*sizeof(xcb_randr_get_output_info_reply_t *));
	if (info == NULL){
		goto E0;
	}
	for (int i = 0; i < len; i++){
		info[i] = NULL;
		info[i] = xcb_randr_get_output_info_reply(conn, cookies[i], NULL);
		if (info[i] != NULL) {
			if (info[i]->crtc != XCB_NONE) {
				cir = xcb_randr_get_crtc_info_reply(conn, xcb_randr_get_crtc_info(conn, info[i]->crtc, XCB_CURRENT_TIME), NULL);
				if (cir != NULL){
					char *cname = (char *) xcb_randr_get_output_info_name(info[i]);
					size_t clen = (size_t) xcb_randr_get_output_info_name_length(info[i]);
					if (strncmp(cname, name, MIN(clen, name_len)) == 0){
						*crtc = info[i]->crtc;
						Found = 1;
						len = i;
						goto E0;
					}
					free(cir);
					cir = NULL;
				}
				
			}
		}
	}
	
E0:
	if (sres != NULL){
		free(sres);
	}
	if (cookies != NULL){
		free(cookies);
	}
	if (info != NULL){
		for (int i = 0; i < len; i++)
			if (info[i] != NULL)
				free(info[i]);
		free(info);
	}
	if (cir != NULL)
		free(cir);
	return Found;	
}

xcb_randr_screen_size_t *get_screen_size(void){
	xcb_generic_error_t *e = NULL;
	xcb_randr_get_screen_info_cookie_t sic = xcb_randr_get_screen_info(conn, root);
	xcb_randr_get_screen_info_reply_t * screen_info_reply = xcb_randr_get_screen_info_reply(conn, sic, &e);
	if (e == NULL){
		fprintf(stderr, "error get screen info");
		return NULL;
	}
	xcb_randr_screen_size_t *scrn_size = xcb_randr_get_screen_info_sizes(screen_info_reply);
	free(reply);
	return scrn_size;
}

int set_crtc_config(xcb_randr_crtc_t crtc, uint16_t rotation){
	xcb_generic_error_t *e = NULL;
	xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(conn, xcb_randr_get_crtc_info(conn, crtc, XCB_CURRENT_TIME), &e);
	int output_len = xcb_randr_get_crtc_info_outputs_length(cir);
	if (e != NULL){
		fprintf(stderr, "error get crtc info");
		return EXIT_FAILURE;
	}
	xcb_randr_output_t *outputs = xcb_randr_get_crtc_info_outputs(cir);
	xcb_randr_set_crtc_config_cookie_t cookie = xcb_randr_set_crtc_config(conn, crtc, XCB_CURRENT_TIME, XCB_CURRENT_TIME, cir -> y, cir->x, cir->mode, XCB_RANDR_ROTATION_ROTATE_90, output_len, outputs);
	e = NULL;
	free(xcb_randr_set_crtc_config_reply(conn, cookie, &e));
	free(cir);
	if (e != NULL){
		fprintf(stderr, "error set crtc config");
		return EXIT_FAILURE;
	}
	xcb_flush(conn);
	return EXIT_SUCCESS;
}

int set_screen_size(uint16_t height, uint16_t width, uint16_t mheight, uint16_t mwidth){
	xcb_randr_set_screen_size(conn, root, height, width, mheight, mwidth);
	xcb_flush(conn);
}
