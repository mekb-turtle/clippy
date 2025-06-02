#ifndef UTIL_H
#define UTIL_H
#define ERR(code, message)         \
	{                              \
		if (msg) *msg = (message); \
		return (code);             \
	}
#endif
