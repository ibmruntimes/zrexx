/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2022. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
*/
#ifndef _ZREXX
#define _ZREXX 1
#include <napi.h>

extern int execute(int fd, const char *, const char *, int, char **);

#if !defined(__MVS__)
#error This addon is for zos only
#endif

#endif
