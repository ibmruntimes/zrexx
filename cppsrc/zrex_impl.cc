#if defined(__MVS__)
#include "zrexx.h"
#include <_Nascii.h>
#include <dynit.h>
#include <mutex>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if ' ' == 0x40
#error Not compiled with -qascii
#endif

// (1) The EXECBLK structure layout
typedef struct EXECBLK_type {
  char EXECBLK_ACRYN[8];
  int EXECBLK_LENGTH;
  int __gap1;
  char EXECBLK_MEMBER[8];
  char EXECBLK_DDNAME[8];
  char EXECBLK_SUBCOM[8];
  void *__ptr32 EXECBLK_DSNPTR;
  int EXECBLK_DSNLEN;
} EXECBLK_type;

// (2) The EVALBLK structure layout
typedef struct EVALBLK_type {
  int EVALBLK_EVPAD1;
  int EVALBLK_EVSIZE;
  int EVALBLK_EVLEN;
  int EVALBLK_EVPAD2;
  char EVALBLK_EVDATA[256];
} EVALBLK_type;

// (3) The EVALBLK structure layout
typedef struct rexplist {
  void *__ptr32 execblk_ptr;
  void *__ptr32 argtable_ptr;
  void *__ptr32 flags_ptr;
  void *__ptr32 instblk_ptr;
  void *__ptr32 res_parms;
  void *__ptr32 evenblk_ptr;
  void *__ptr32 reserved_workarea_ptr;
  void *__ptr32 reserved_userfield_ptr;
  void *__ptr32 reserved_envblock_ptr;
  void *__ptr32 rexx_return_code_ptr;
} rexplist_t;

// (4) The fixed size part of below the bar storage layout.
typedef struct workarea {
  void *__ptr32 execblk_ptr;
  void *__ptr32 argtable_ptr;
  unsigned int flags;
  void *__ptr32 instblk_ptr;
  void *__ptr32 res_parm5;
  void *__ptr32 evalblk_ptr;
  void *__ptr32 reserved_workarea_ptr;
  void *__ptr32 reserved_userfield_ptr;
  void *__ptr32 reserved_envblock_ptr;
  void *__ptr32 rexx_return_code_ptr;
  int rexx_return_code;
  char *__ptr32 addr_of_parm_list;
  char *__ptr32 __dcb_address;
  char irxexec[8];
  EXECBLK_type exec_blk;
  EVALBLK_type eval_blk;
  rexplist_t plist;
  char dataset[56];
  int fd;
  int dsa[18];
} workarea_t;

// (5) a mutex to serialized DDNAME allocation plus dellocation
static std::mutex access_lock;

// (6) Allocated DDNAME SYSEXEC to the DATASET name of the PDS
static int alloc_sysexec(const char *dsname, int verbose) {
  int len = strlen(dsname);
  __dyn_t ip;

  dyninit(&ip);
  char *name = (char *)alloca(len + 1);
  memcpy(name, dsname, len + 1);
  ip.__ddname = (char *)"SYSEXEC";
  ip.__dsname = name;
  ip.__status = __DISP_SHR;
  int rc = dynalloc(&ip);
  if (rc != 0 && verbose) {
    fprintf(stderr,
            "Dynalloc of SYSEXEC with dataset '%s' failed with "
            "error code %d, info code %d\n",
            dsname, ip.__errcode, ip.__infocode);
  }
  return rc;
}

// (7) Allocated DDNAME SYSTSIN to DUMMY
static int alloc_systsin(int verbose) {
  __dyn_t ip;

  dyninit(&ip);
  ip.__ddname = (char *)"SYSTSIN";
  ip.__status = __DISP_MOD;
  ip.__misc_flags = __DUMMY_DSN;
  int rc = dynalloc(&ip);
  if (rc != 0 && verbose) {
    fprintf(stderr,
            "Dynalloc of SYSTSIN failed "
            "error code %d, info code %d\n",
            ip.__errcode, ip.__infocode);
  }
  return rc;
}

// (8) Allocated DDNAME SYSTSPRT to UNIT=SYSDA
static int alloc_systsprt(int verbose) {
  __dyn_t ip;

  dyninit(&ip);
  ip.__ddname = (char *)"SYSTSPRT";
  ip.__status = __DISP_NEW;
  ip.__normdisp = __DISP_DELETE;
  ip.__conddisp = __DISP_DELETE;
  ip.__unit = (char *)"SYSDA";
  ip.__dsorg = __DSORG_PS;
  ip.__recfm = _VB_;
  int rc = dynalloc(&ip);
  if (rc != 0 && verbose) {
    fprintf(stderr,
            "Dynalloc of systsprt failed "
            "error code %d, info code %d\n",
            ip.__errcode, ip.__infocode);
  }
  return rc;
}

// (9) DEALLOCATE DDNAME
static int dealloc_dd(char *ddname, int verbose) {
  __dyn_t ip;

  dyninit(&ip);
  ip.__ddname = ddname;
  int rc = dynfree(&ip);
  if (rc != 0 && verbose) {
    fprintf(stderr,
            "Dynfree of %s failed "
            "error code %d, info code %d\n",
            ddname, ip.__errcode, ip.__infocode);
  }
  return rc;
}

// (10) Fill in the EXECBLK structure
static void *execblock(EXECBLK_type *exec_blk, char *__ptr32 dsn,
                       const char *member_name) {
  memset(exec_blk, 0, sizeof(EXECBLK_type));
  memcpy(exec_blk->EXECBLK_ACRYN, "\xc9\xd9\xe7\xc5\xe7\xc5\xc3\xc2", 8);
  exec_blk->EXECBLK_LENGTH = sizeof(EXECBLK_type);

  int len = strlen(member_name);
  if (len > 8)
    len = 8;
  memcpy(exec_blk->EXECBLK_MEMBER, member_name, len);
  __a2e_l(exec_blk->EXECBLK_MEMBER, len);
  if (len < 8) {
    memset(exec_blk->EXECBLK_MEMBER + len, 0x40, 8 - len);
  }

  memset(exec_blk->EXECBLK_SUBCOM, 0x40, 8);

  if (dsn) {
    exec_blk->EXECBLK_DSNPTR = dsn;
    exec_blk->EXECBLK_DSNLEN = strlen(dsn);
  }
  return exec_blk;
}

// (11) Fill in the EVALBLK structure
static void *evalblock(EVALBLK_type *evalblk) {
  memset(evalblk, 0, sizeof(EVALBLK_type));
  evalblk->EVALBLK_EVLEN = sizeof(EVALBLK_type);
  evalblk->EVALBLK_EVSIZE = (sizeof(EVALBLK_type) >> 3);
  return evalblk;
}

// (12) Fill in the parameter list (OS style) set HOB for the last argeument
// address
static rexplist_t *
plist(rexplist_t *res, void *__ptr32 execblk_ptr, void *__ptr32 argtable_ptr,
      void *__ptr32 flags_ptr, void *__ptr32 instblk_ptr,
      void *__ptr32 res_parms, void *__ptr32 evenblk_ptr,
      void *__ptr32 reserved_workarea_ptr, void *__ptr32 reserved_userfield_ptr,
      void *__ptr32 reserved_envblock_ptr, void *__ptr32 rexx_return_code_ptr) {
  res->execblk_ptr = execblk_ptr;
  res->argtable_ptr = argtable_ptr;
  res->flags_ptr = flags_ptr;
  res->instblk_ptr = instblk_ptr;
  res->res_parms = res_parms;
  res->evenblk_ptr = evenblk_ptr;
  res->reserved_workarea_ptr = reserved_workarea_ptr;
  res->reserved_userfield_ptr = reserved_userfield_ptr;
  res->reserved_envblock_ptr = reserved_envblock_ptr;
  res->rexx_return_code_ptr = rexx_return_code_ptr;
  *((unsigned int *)&res->rexx_return_code_ptr) |= 0x80000000;
  return res;
}

// (13) ISSUE svc6, ie, the LINK macro
static int svc6(void *reg15, void *reg1, void *dsa) {
  __asm(" sam31 \n"
        " svc 6\n"
        " sam64 \n"
        : "+NR:r15"(reg15)
        : "NR:r1"(reg1), "NR:r13"(dsa), "NR:r0"(0)
        :);
  return (int)(long)reg15;
}

// (14) Allocate storage and create the ARGUMENT entries block (variable size)
static void *rexxargs(int argc, char **argv) {
  int i;
  typedef struct argentry {
    char *__ptr32 arg;
    int arg_len;
  } argentry_t;
  char *start;
  int argtable_size = (argc + 1) * sizeof(argentry_t);
  int argbuffer_size = 0;
  for (i = 0; i < argc; ++i) {
    argbuffer_size += strlen(argv[i]) + 1;
  }
  char *res = (char *)__malloc31(argtable_size + argbuffer_size);
  char *buffer = res + argtable_size;
  argentry_t *arg = (argentry_t *)res;
  start = buffer;
#if __VARGV
  // this does not work, apparently under the TSO environment, everything must
  // be stuffed into one argument separated by space, unfortunately
  for (i = 0; i < argc; ++i) {
    int len = strlen(argv[i]);
    arg[i].arg = start;
    arg[i].arg_len = len;
    memcpy(start, argv[i], len + 1);
    start += (len + 1);
  }
  arg[i].arg = (char *__ptr32)0xffffffff;
  arg[i].arg_len = 0xffffffff;
#else
  arg[0].arg = start;
  for (i = 0; i < argc; ++i) {
    int len = strlen(argv[i]);
    memcpy(start, argv[i], len);
    start[len] = ' ';
    start += (len + 1);
  }
  --start;
  arg[0].arg_len = start - arg[0].arg;
  arg[1].arg = (char *__ptr32)0xffffffff;
  arg[1].arg_len = 0xffffffff;
#endif
  __a2e_l(buffer, start - buffer);
  return res;
}

// (15) class for allocation and de-allocation of DDNAMEs needed for IRXEXEC
class rexx_dd {
  int error;

public:
  rexx_dd(const char *dsn) : error(0) {
    int rc;
    rc = alloc_sysexec(dsn, 1);
    if (rc != 0)
      error = 1;
    rc = alloc_systsin(1);
    if (rc != 0)
      error = 1;
    rc = alloc_systsprt(1);
    if (rc != 0)
      error = 1;
  }
  bool inerror(void) { return error == 1; }
  ~rexx_dd() {
    int rc;
    rc = dealloc_dd((char *)"SYSTSPRT", 1);
    if (rc != 0)
      error = 1;
    rc = dealloc_dd((char *)"SYSTSIN", 1);
    if (rc != 0)
      error = 1;
    rc = dealloc_dd((char *)"SYSEXEC", 1);
    if (rc != 0)
      error = 1;
  }
};

// (16) class to manage the release of below the bar storage.
class ptr32_holder {
  void *p;

public:
  ptr32_holder(void *ptr) : p(ptr) {}
  ~ptr32_holder() { free(p); }
};

// (17) internal function to actually run IRXEXEC
static int callrexx_i(int output_fd, const char *dsn, const char *mem, int argc,
                      char **argv) {
  int i, rc;
  rexx_dd dd_allocation(dsn);

  if (dd_allocation.inerror())
    return -1;

  workarea_t *wa = (workarea_t *)__malloc31(sizeof(workarea_t));
  ptr32_holder wa_stor(wa);
  snprintf(wa->dataset, 55, "%s", dsn);
  __a2e_s(wa->dataset);
  wa->execblk_ptr = execblock(&(wa->exec_blk), wa->dataset, mem);
  wa->flags = 0x50000000;
  wa->instblk_ptr = 0;
  wa->res_parm5 = 0;
  wa->evalblk_ptr = evalblock(&(wa->eval_blk));
  wa->reserved_workarea_ptr = 0;
  wa->reserved_userfield_ptr = 0;
  wa->reserved_envblock_ptr = 0;
  wa->rexx_return_code_ptr = &(wa->rexx_return_code);
  memcpy(wa->irxexec, "\xC9\xD9\xE7\xC5\xE7\xC5\xC3\x40", 8);
  wa->argtable_ptr = rexxargs(argc, argv);
  ptr32_holder rexx_args_stor(wa->argtable_ptr);
  rexplist_t *parms =
      plist(&(wa->plist), &(wa->execblk_ptr), &(wa->argtable_ptr), &(wa->flags),
            &(wa->instblk_ptr), &(wa->res_parm5), &(wa->evalblk_ptr),
            &(wa->reserved_workarea_ptr), &(wa->reserved_userfield_ptr),
            &(wa->reserved_envblock_ptr), &(wa->rexx_return_code_ptr));
  wa->addr_of_parm_list = wa->irxexec;
  wa->__dcb_address = 0;
  char rexrc[32];
  rc = svc6(&(wa->addr_of_parm_list), parms, wa->dsa);
  if (rc != 0) {
    fprintf(stderr, "svc 6 rc %d\n", rc);
    return -1;
  }
  if (wa->rexx_return_code != 0) {
    fprintf(stderr, "REXX call rc %d\n", wa->rexx_return_code);
    return -1;
  }
  char *rexrc_e = ((EVALBLK_type *)(wa->evalblk_ptr))->EVALBLK_EVDATA;
  int rexrclen = ((EVALBLK_type *)(wa->evalblk_ptr))->EVALBLK_EVLEN;
  memcpy(rexrc, rexrc_e, rexrclen);
  __e2a_l(rexrc, rexrclen);
  rexrc[rexrclen] = 0;
  FILE *f = fopen("DD:SYSTSPRT", "rb,type=record");
  if (f) {
    int bytes;
    char buffer[4096];
    bytes = fread(buffer, 1, 4096, f);
    while (bytes > 0) {
      __e2a_l(buffer, bytes);
      write(wa->fd, buffer, bytes);
      write(wa->fd, "\x0a", 1);
      bytes = fread(buffer, 1, 4096, f);
    }
    fclose(f);
  }
  rc = atoi(rexrc);
  return rc;
}
// (18) class to manage the setting of ascii runmode on entry and exit.
class ascii_runmode {
  int mode;

public:
  ascii_runmode() { mode = __ae_thread_swapmode(__AE_ASCII_MODE); }
  ~ascii_runmode() { __ae_thread_swapmode(mode); }
};

// (19) the external interface that is used by the addon.
extern int execute(int output_fd, const char *dsn, const char *mem,
                          int argc, char **argv) {
  std::lock_guard<std::mutex> guard(access_lock);
  ascii_runmode runmode;
  int res = callrexx_i(output_fd, dsn, mem, argc, argv);
  return res;
};

#else
#error this addon is for zos only
#endif
