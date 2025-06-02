#include "copy.h"
#include "util.h"

#include <stdlib.h>
#include "copy-wayland.h"
#include "copy-x11.h"

int copy(bool primary, struct copy_data *data, char **msg, int (*pre_loop_callback)(void *ctx), void *ctx) {
	char *display_name = getenv("WAYLAND_DISPLAY");
	if (display_name && *display_name)
		return copy_wayland(display_name, primary, data, msg, pre_loop_callback, ctx);

	display_name = getenv("DISPLAY");
	if (display_name && *display_name)
		return copy_x11(display_name, primary, data, msg, pre_loop_callback, ctx);

	ERR(1, "No display found");
}
