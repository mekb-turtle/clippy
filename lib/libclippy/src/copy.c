#include "../include/copy.h"

#include <stdlib.h>
#include "../include/copy-wayland.h"
#include "../include/copy-x11.h"

#define ERR()

int copy(bool primary, struct copy_data data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
	char *display_name = NULL;

#ifdef HAS_WAYLAND
	display_name = getenv("WAYLAND_DISPLAY");
	if (display_name && *display_name) {
		return copy_wayland(display_name, primary, data, error_msg, pre_loop_callback, ctx);
	}
#endif

#ifdef HAS_X11
	display_name = getenv("DISPLAY");
	if (display_name && *display_name) {
		return copy_x11(display_name, primary, data, error_msg, pre_loop_callback, ctx);
	}
#endif

	if (error_msg) *error_msg = "No compatible clipboard implementation found";
	return 1;
}

const char *copy_get_mime(enum copy_type type) {
	switch (type) {
		case COPY_TYPE_TEXT_UTF8:
			return "text/plain";
		case COPY_TYPE_IMAGE_PNG:
			return "image/png";
		case COPY_TYPE_IMAGE_JPEG:
			return "image/jpeg";
		case COPY_TYPE_IMAGE_WEBP:
			return "image/webp";
		default:
			return NULL;
	}
}