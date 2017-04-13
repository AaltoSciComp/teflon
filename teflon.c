/*
 * Teflon: prevent programs from changing sticky bits on files.
 *
 * Modeled with pride from eatmydata:
 * https://github.com/stewartsmith/libeatmydata/blob/master/libeatmydata/libeatmydata.c
 *
 * Copyright Richard Darst.
 * GPLv3+
 *
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static STICKY = S_ISGID;
#define NEW_MODE(buf, mode) ((buf.st_mode & STICKY) | (mode & ~STICKY));

// These automatically find the original function and set it in the
// libc_$NAME variable.  Copied from eatmydata.
#define ASSIGN_DLSYM_OR_DIE(name)\
  libc_##name = (libc_##name##_t)(intptr_t)dlsym(RTLD_NEXT, #name);\
  if (!libc_##name || dlerror())\
    _exit(1);

#define ASSIGN_DLSYM_IF_EXIST(name)\
  libc_##name = (libc_##name##_t)(intptr_t)dlsym(RTLD_NEXT, #name);\
  dlerror();

// All the original functions we override.
typedef int (*libc_chmod_t)(const char *pathname, mode_t mode);
typedef int (*libc_fchmod_t)(int fd, mode_t mode);
typedef int (*libc_fchmodat_t)(int dirfd, const char *pathname, mode_t mode, int flags);
typedef int (*libc_chown_t)(const char *pathname, uid_t owner, gid_t group);
typedef int (*libc_fchown_t)(int fd, uid_t owner, gid_t group);
typedef int (*libc_lchown_t)(const char *pathname, uid_t owner, gid_t group);
typedef int (*libc_fchownat_t)(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
static libc_chmod_t libc_chmod = NULL;
static libc_fchmod_t libc_fchmod = NULL;
static libc_fchmodat_t libc_fchmodat = NULL;
static libc_chown_t libc_chown = NULL;
static libc_fchown_t libc_fchown = NULL;
static libc_lchown_t libc_lchown = NULL;
static libc_fchownat_t libc_fchownat = NULL;

// Initialize
void __attribute__ ((constructor)) eatmydata_init(void)
{
  ASSIGN_DLSYM_OR_DIE(chmod);
  ASSIGN_DLSYM_OR_DIE(fchmod);
  ASSIGN_DLSYM_OR_DIE(fchmodat);
  ASSIGN_DLSYM_OR_DIE(chown);
  ASSIGN_DLSYM_OR_DIE(fchown);
  ASSIGN_DLSYM_OR_DIE(lchown);
  ASSIGN_DLSYM_OR_DIE(fchownat);
}

/*
 * Chmod functions: stat the file, change sticky bit to what it was before.
 */
int chmod(const char *pathname, mode_t mode) {
  struct stat buf;
  int ret = stat(pathname, &buf);
  if (ret != 0) { return ret; }
  mode = NEW_MODE(buf, mode)
  return libc_chmod(pathname, mode);
}

int fchmod(int fd, mode_t mode) {
  struct stat buf;
  int ret = fstat(fd, &buf);
  if (ret != 0) { return ret; }
  mode = NEW_MODE(buf, mode)
  return libc_fchmod(fd, mode);
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
  struct stat buf;
  int ret = fstatat(dirfd, pathname, &buf, flags);
  if (ret != 0) { return ret; }
  mode = NEW_MODE(buf, mode)
  return libc_fchmodat(dirfd, pathname, mode, flags);
}

/*
 * Chown functions: remove group
 */
int chown(const char *pathname, uid_t owner, gid_t group) {
  return libc_chown(pathname, owner, -1);
}

int fchown(int fd, uid_t owner, gid_t group) {
  return libc_fchown(fd, owner, -1);
}

int lchown(const char *pathname, uid_t owner, gid_t group) {
  return libc_lchown(pathname, owner, -1);
}

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
  return libc_fchownat(dirfd, pathname, owner, -1, flags);
}
