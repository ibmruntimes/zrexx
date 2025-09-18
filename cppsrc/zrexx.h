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
#error This addon is for z/OS only
#endif

// XL-specific NR parameter constraint:
// https://www.ibm.com/docs/en/zos/3.1.0?topic=statements-inline-assembly-extension
#if __clang_major__ < 18
#define ZREXX_NR(attr,reg) attr "NR:" #reg
#else
#define ZREXX_NR(attr,reg) attr "{" #reg "}"
#endif 

#endif
