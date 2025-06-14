#ifndef UTIL_H
#define UTIL_H
#define ERR_RET(code, message)         \
	{                              \
		if (error_msg) *error_msg = (message); \
		return (code);             \
	}
#define ERR(code, message)        \
	{                              \
		if (error_msg) *error_msg = (message); \
		res = (code);              \
		goto cleanup;              \
	}
#endif
