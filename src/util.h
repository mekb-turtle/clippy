#ifndef UTIL_H
#define UTIL_H
#define ERR(code, message)         \
	{                              \
		if (msg) *msg = (message); \
		return (code);             \
	}
#define FORK()                                       \
	{                                                \
		pid_t pid = fork();                          \
		if (pid == -1) ERR(errno, "Failed to fork"); \
		if (pid != 0) return 0;                      \
	}
#endif
