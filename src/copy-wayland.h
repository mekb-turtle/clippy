#ifndef COPY_WAYLAND
#define COPY_WAYLAND
#include <stddef.h>
#include <stdbool.h>
#include "copy.h"
int copy_wayland(char *display_name, bool fork, bool primary, struct copy_data *data, char **msg);
#endif
