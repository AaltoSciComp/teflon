# Teflon: LD_PRELOAD to prevent manipulations of setgid bit and gid

This library uses LD_PRELOAD to override the chmod and chown family of
functions, so that programs can not:
- Change the setgid bit of files
- Change the group of files

The purpose is usage on filesystems where quota is by groups, not by
users, and use the setgid bit to preserve the group of newly
created files.

This is still conceptual, and both the code and support scripts are
not well tested.

These functions are overridden:
- chmod, fchmod, fchmodat
  - These result in an extra stat call to find the previous setgid bit
    status, then insert this into the requested mode using bit operators.
- chown, fchown, lchown, fchownat
  - These change the group option to -1 unconditionally, so that the
    group is never changed.


# Installation

Run make, then copy the resulting `teflon` program to a place to be
used.  This is both the LD_PRELOAD library and wrapper to execute it.


# References
- Inspired and adapted from eatmydata:
  - https://github.com/stewartsmith/libeatmydata/
  - https://github.com/stewartsmith/libeatmydata/blob/master/libeatmydata/libeatmydata.c
