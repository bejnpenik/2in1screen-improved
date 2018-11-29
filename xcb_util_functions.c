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
	if (conn)
		xcb_disconnect(conn);
}


int get_screen(void){
	scrn = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	if (scrn == NULL){
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


int get_crtc_with_name(xcb_randr_crtc_t *crtc, const char *name, const size_t name_len){
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
						len = ++i;
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

xcb_randr_screen_size_t *get_screen_size(xcb_window_t win){
	xcb_randr_get_screen_info_cookie_t sic = xcb_randr_get_screen_info(conn, win);
	xcb_randr_get_screen_info_reply_t * screen_info_reply = xcb_randr_get_screen_info_reply(conn, sic, NULL);
	xcb_randr_screen_size_t *scrn_size = NULL; //xcb_randr_get_screen_info_sizes(screen_info_reply);
	scrn_size = (xcb_randr_screen_size_t *)malloc(sizeof(xcb_randr_screen_size_t));
	memcpy(scrn_size, xcb_randr_get_screen_info_sizes(screen_info_reply), sizeof(xcb_randr_screen_size_t));
	free(screen_info_reply);
	return scrn_size;
}
int monitor_with_name_disconnected(const char *name, size_t name_len){
	int len;
	int disconnected = 0;
	int i;
	xcb_randr_get_screen_resources_reply_t *sres = NULL;
	xcb_randr_get_output_info_cookie_t *cookies = NULL;
	xcb_randr_get_output_info_reply_t **info = NULL;
	sres = xcb_randr_get_screen_resources_reply(conn, xcb_randr_get_screen_resources(conn, scrn->root), NULL);
	len = xcb_randr_get_screen_resources_outputs_length(sres);
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(sres);
	cookies = (xcb_randr_get_output_info_cookie_t *) malloc(len*sizeof(xcb_randr_get_output_info_cookie_t));
	for (int i = 0; i < len; i++) {
		cookies[i] = xcb_randr_get_output_info(conn, outputs[i], XCB_CURRENT_TIME);
	}
	info = (xcb_randr_get_output_info_reply_t **)malloc(len*sizeof(xcb_randr_get_output_info_reply_t *));
	for (i = 0; i < len; i++){
		info[i] = NULL;
		info[i] = xcb_randr_get_output_info_reply(conn, cookies[i], NULL);
		char *oname = xcb_randr_get_output_info_name(info[i]);
		int oname_len = xcb_randr_get_output_info_name_length(info[i]);
		if (strncmp(oname, name, MIN(oname_len, name_len))==0){
			if (info[i]->crtc != XCB_NONE && info[i]->connection == XCB_RANDR_CONNECTION_CONNECTED) {
				disconnected = 0;
				goto cleanup;
			}else{
				disconnected = 1;
				goto cleanup;
			}
		}
	}
cleanup:
	if (sres != NULL){
		free(sres);
	}
	if (cookies != NULL){
		free(cookies);
	}
	if (info != NULL){
		for (int j = 0; j < i+1; j++)
			if (info[j] != NULL)
				free(info[i]);
		free(info);
	}
	
	return disconnected;
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
	xcb_randr_set_crtc_config_cookie_t cookie = xcb_randr_set_crtc_config(conn, crtc, XCB_CURRENT_TIME, XCB_CURRENT_TIME, cir -> y, cir->x, cir->mode, rotation, output_len, outputs);
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
	xcb_randr_set_screen_size(conn, root, width, height, mwidth, mheight);
	xcb_flush(conn);
	return EXIT_SUCCESS;
}

void set_screen_config(uint16_t rotation){
	xcb_randr_get_screen_info_cookie_t sic = xcb_randr_get_screen_info(conn, scrn->root);
	xcb_randr_get_screen_info_reply_t * screen_info_reply = xcb_randr_get_screen_info_reply(conn, sic, NULL);
	xcb_randr_set_screen_config_cookie_t ssic = xcb_randr_set_screen_config(conn, scrn->root, XCB_CURRENT_TIME, XCB_CURRENT_TIME, screen_info_reply->sizeID, rotation, screen_info_reply->rate);
	free(xcb_randr_set_screen_config_reply(conn, ssic, NULL));
	free(screen_info_reply);
	xcb_flush(conn);
	
}

xcb_input_device_info_t *find_device_info(const char *dev_name, const size_t dev_name_len){
	xcb_generic_error_t *e = NULL;
	char *name = NULL;
	int name_l;
	xcb_input_list_input_devices_cookie_t cookie = xcb_input_list_input_devices(conn);
	xcb_input_list_input_devices_reply_t *reply = xcb_input_list_input_devices_reply(conn, cookie, &e);
	xcb_input_device_info_t *tmp = NULL;
	if (e != NULL) {
		fprintf(stderr, "Cant get device list reply!\n");
		return NULL;
	}
	xcb_input_device_info_t *devices = xcb_input_list_input_devices_devices(reply);
	xcb_input_xi_query_device_cookie_t cookies;
	xcb_input_xi_query_device_reply_t *replies = NULL;
	for (int i = 0; i < reply->devices_len; i++){
		e = NULL;
		cookies = xcb_input_xi_query_device(conn, devices[i].device_id);
		replies = xcb_input_xi_query_device_reply(conn, cookies, &e);
		if (e != NULL){
			fprintf(stderr, "Error querying %d reply!\n", i);
			continue;
		}
		xcb_input_xi_device_info_iterator_t iterator = xcb_input_xi_query_device_infos_iterator(replies);
		if (iterator.data -> type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER || iterator.data -> type == XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER){
			name  = xcb_input_xi_device_info_name(iterator.data);
			name_l = xcb_input_xi_device_info_name_length(iterator.data);
			if (strncmp(name, dev_name, MIN(dev_name_len, name_l)) == 0){
				tmp = (xcb_input_device_info_t *)malloc(sizeof(xcb_input_device_info_t));
				memcpy(tmp, devices+i, sizeof(xcb_input_device_info_t));
				free(replies);
				free(reply);
				return tmp;
			}
		}
		free(replies);
	}
	free(reply);
	return NULL;
}

int set_device_property(const char *device_name, const size_t device_name_len, const char *property_name, const size_t property_name_len, const char *type_name, const size_t type_name_len, int format,  const int item_nbr, const void *items){
	xcb_generic_error_t *e = NULL;
	xcb_input_device_info_t *device_info = find_device_info(device_name, device_name_len);
	if (e != NULL){
		fprintf(stderr, "Error querying device info %s", device_name);
		return EXIT_FAILURE;
	}
	//printf("Device id: %d\n", device_info->device_id);
	xcb_atom_t property = get_atom_by_name(property_name, property_name_len);
	xcb_input_xi_get_property_cookie_t get_prop_cookie = xcb_input_xi_get_property(conn, device_info -> device_id, 0,  property, XCB_ATOM_ANY, 0, 1000);
	xcb_input_xi_get_property_reply_t  *get_prop_reply =  xcb_input_xi_get_property_reply(conn, get_prop_cookie, &e);
	if (e != NULL){
		fprintf(stderr, "Error querying property %s on device %s", property_name, device_name);
		return EXIT_FAILURE;
	}
	xcb_atom_t type = get_atom_by_name(type_name, type_name_len);
	//if (type != get_prop_reply -> type){
	//	type = get_prop_reply -> type;
	//}
	xcb_input_xi_change_property(conn, device_info->device_id, XCB_PROP_MODE_REPLACE, format,  property, type, item_nbr, items);
	xcb_flush(conn);
	free(device_info);
	free(get_prop_reply);
	return EXIT_SUCCESS;
}

int set_device_property_id(xcb_input_device_info_t *device_info, const char *property_name, const size_t property_name_len, const char *type_name, const size_t type_name_len, int format,  const int item_nbr, const void *items){
	xcb_atom_t property = get_atom_by_name(property_name, property_name_len);
	xcb_input_xi_get_property_cookie_t get_prop_cookie = xcb_input_xi_get_property(conn, device_info -> device_id, 0,  property, XCB_ATOM_ANY, 0, 1000);
	xcb_input_xi_get_property_reply_t  *get_prop_reply =  xcb_input_xi_get_property_reply(conn, get_prop_cookie, NULL);
	xcb_atom_t type = get_atom_by_name(type_name, type_name_len);
	//if (type != get_prop_reply -> type){
	//	type = get_prop_reply -> type;
	//}
	xcb_input_xi_change_property(conn, device_info->device_id, XCB_PROP_MODE_REPLACE, format,  property, type, item_nbr, items);
	xcb_flush(conn);
	free(get_prop_reply);
	return EXIT_SUCCESS;
}
