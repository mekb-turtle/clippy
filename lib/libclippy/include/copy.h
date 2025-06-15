#ifndef COPY_H
#define COPY_H
#include <stddef.h>
#include <stdbool.h>

struct copy_data {
	char *mime;
	size_t size;
	void *data;
};

int copy(bool primary, struct copy_data *data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx);
#endif
