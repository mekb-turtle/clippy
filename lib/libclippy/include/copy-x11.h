#ifndef COPY_X11
#define COPY_X11
#include "copy.h"

int copy_x11(char *display_name, bool primary, struct copy_data data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx);
#endif

