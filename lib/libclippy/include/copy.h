#ifndef COPY_H
#define COPY_H
#include <stddef.h>
#include <stdbool.h>

struct copy_data {
	enum copy_type {
		COPY_TYPE_UNKNOWN = 0,
		COPY_TYPE_TEXT_UTF8,
		COPY_TYPE_IMAGE_PNG,
		COPY_TYPE_IMAGE_JPEG,
		COPY_TYPE_IMAGE_WEBP
	} type;
	size_t size;
	void *data;
};

int copy(bool primary, struct copy_data data, char **error_msg, int (*pre_loop_callback)(void *ctx), void *ctx);
const char *copy_get_mime(enum copy_type type);
#endif