#ifndef _ZREXX
#define _ZREXX 1
#include <napi.h>

extern int execute(int fd, const char *, const char *, int, char **);

#if !defined(__MVS__)
#error This addon is for zos only
#endif

#endif
