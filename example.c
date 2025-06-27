#include "lib/libclippy/include/copy.h"

#include <string.h>
#include <stdio.h>

extern unsigned char test_png[];
extern unsigned int test_png_len;

// We don't actually need to error, just need to return from function in the parent process
static char *parent_process = "Parent process";

static int do_fork(void *msg_) {
	// #define DO_FORK
	printf("Serving clipboard\n");
#ifdef DO_FORK
	char **error_msg = msg_;
	pid_t pid = fork();
	if (pid == -1) ERR_RET(errno, "Failed to fork");
	if (pid != 0) ERR_RET(1, parent_process);
#else
	(void) msg_;
#endif
	return 0;
}

int main(void) {
	printf("Starting...\n");
	char *msg = NULL;
	if (copy(false,
	         (struct copy_data) {
#define TEXT(str) strlen(str), str
// #define COPY_IMAGE
#ifdef COPY_IMAGE
	                 COPY_TYPE_IMAGE_PNG,
	                 test_png_len,
	                 test_png,
#else
	                 COPY_TYPE_TEXT_UTF8,
	                 TEXT("hello world!"),
#endif
	         },
	         &msg, &do_fork, &msg)) {
		if (msg == parent_process) return 0;
		fprintf(stderr, "copy: %s", msg);
		return 1;
	}

	return 0;
}
