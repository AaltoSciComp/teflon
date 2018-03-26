/*
 * Teflon: prevent programs from changing setgid bits or groups on files.
 *
 * Modeled with pride and derived from eatmydata:
 * https://github.com/stewartsmith/libeatmydata/blob/master/libeatmydata/libeatmydata.c
 *
 * Copyright Richard Darst
 * Created at Aalto University
 * GPLv3+
 *
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// The setgid bit: 02000
#define SETGID S_ISGID
// This macro takes old mode and replaces just the setgid with
// the value from the stat results buf.
#define NEW_MODE(buf, mode) ((buf.st_mode & SETGID) | (mode & ~SETGID));

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
 * Chmod functions: stat the file, change setgid bit to what it was
 * before.  This requires statting the original file again to get the
 * previous value of the bit.
 */
int chmod(const char *pathname, mode_t mode) {
  struct stat buf;
  int ret = stat(pathname, &buf);
  // How do we handle errors?  For now, simply do nothing.
  if (ret == 0)
    mode = NEW_MODE(buf, mode);
  return libc_chmod(pathname, mode);
}

int fchmod(int fd, mode_t mode) {
  struct stat buf;
  int ret = fstat(fd, &buf);
  if (ret == 0)
    mode = NEW_MODE(buf, mode);
  return libc_fchmod(fd, mode);
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
  struct stat buf;
  int ret = fstatat(dirfd, pathname, &buf, flags);
  if (ret == 0)
    mode = NEW_MODE(buf, mode);
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


int main(int argc, char **argv) {
  //printf("%d %s %s\n", argc, argv[0], argv[1]);
  // Usage
  if (argc == 1 || (argc>1 && strcmp(argv[1], "-h")==0) || (argc>1 && strcmp(argv[1], "--help")==0)) {
    printf("Usage: %s program [args [...]]\n", argv[0]);
    exit(1);
  }

  // Run with teflon
  // Merge this program's path to LD_PRELOAD if it exists already.
  char *ld_preload = argv[0];
  char *old_ld_preload = getenv("LD_PRELOAD");
  if (old_ld_preload != NULL) {
    char *ld_preload2 = malloc(strlen(ld_preload)+strlen(old_ld_preload)+2);
    ld_preload = ld_preload2;  // Somehow needed to prevent segfaults.
    strcat(ld_preload, argv[0]);
    strcat(ld_preload+strlen(argv[0]), ":");
    strcat(ld_preload+strlen(argv[0])+1, old_ld_preload);
  }

  // Run the program
  setenv("LD_PRELOAD", ld_preload, 1);
  execvp(argv[1], argv+1);

  // Only get here if exec fails.
  fprintf(stderr, "%s FAILED: could not exec %s (%d: %s)\n",
	  argv[0], argv[1], errno, strerror(errno));
  return 2;
}
