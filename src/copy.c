#include "copy.h"
#include <stdlib.h>
#include "copy-wayland.h"
#include "copy-x11.h"
#include "util.h"
int copy(bool fork, bool primary, struct copy_data *data, char **msg) {
	char *display_name = getenv("WAYLAND_DISPLAY");
	if (display_name)
		return copy_wayland(display_name, fork, primary, data, msg);

	display_name = getenv("DISPLAY");
	if (display_name)
		return copy_x11(display_name, fork, primary, data, msg);

	ERR(1, "No display found");
}
