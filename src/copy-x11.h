#ifndef COPY_X11
#define COPY_X11
#include <stddef.h>
#include <stdbool.h>
#include "copy.h"
int copy_x11(char *display_name, bool fork, bool primary, struct copy_data *data, char **msg);
#endif
