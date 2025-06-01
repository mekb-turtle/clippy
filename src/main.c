#include "copy.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include <err.h>
#include <string.h>

extern unsigned char test_png[];
extern unsigned int test_png_len;

int main(void) {
	char *msg = NULL;
	if (copy(true, false,
	         (struct copy_data[]) {
#define TEXT(str) strlen(str), str
	                 {"image/png", test_png_len, test_png},
	                 {"text/plain", TEXT("hello world")},
                     {0}
    },
	         &msg))
		errx(1, "copy: %s", msg);

	return 0;
}