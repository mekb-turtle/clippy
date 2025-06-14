#ifndef UTIL_H
#define UTIL_H
#define ERR(code, message)                     \
	{                                          \
		if (error_msg) *error_msg = (message); \
		res = (code);                          \
		goto cleanup;                          \
	}
#endif