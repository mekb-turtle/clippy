#include "copy.h"
#include "util.h"

#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include <err.h>
#include <string.h>

extern unsigned char test_png[];
extern unsigned int test_png_len;

// We don't actually need to error, just need to return from function in the parent process
static char *parent_process = "Parent process";

static int do_fork(void *msg_) {
	char **msg = msg_;
	pid_t pid = fork();
	if (pid == -1) ERR(errno, "Failed to fork");
	if (pid != 0) ERR(1, parent_process);
	return 0;
}

int main(void) {
	char *msg = NULL;
	if (copy(false,
	         (struct copy_data[]) {
#define TEXT(str) strlen(str), str
	                 {"image/png", test_png_len, test_png},
	                 {"text/plain", TEXT("hello world")},
	                 {0}
    },
	         &msg, &do_fork, &msg)) {
		if (msg == parent_process) return 0;
		errx(1, "copy: %s", msg);
	}

	return 0;
}
