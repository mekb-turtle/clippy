#include "copy.h"

#include <stdlib.h>
#include "copy-wayland.h"
#include "copy-x11.h"

#define ERR()

int copy(bool primary, struct copy_data *data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
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