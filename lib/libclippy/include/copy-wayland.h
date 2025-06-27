#ifndef COPY_WAYLAND
#define COPY_WAYLAND
#include "copy.h"

int copy_wayland(char *display_name, bool primary, struct copy_data data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx);
#endif