# Teflon: LD_PRELOAD to prevent manipulations of stick bit and groups

This library uses LD_PRELOAD to override the chmod and chown family of
functions, so that programs can not:
- Change the sticky bit of files
- Change the group of files

The purpose is usage on filesystems where quota is by groups, not by
users, and use the group sticky bit to preserve the group of newly
created files.

This is still conceptual, and both the code and support scripts are
not well tested.

These functions are overridden:
- chmod, fchmod, fchmodat
  - These result in an extra stat call
- chown, fchown, lchown, fchownat
  - These change 

# Installation

Run make, then copy the teflon shell script and teflon.so to the same
directory.  Run the shell script.  Currently there is nothing more
advanced.


# References
- Inspired and adapted from eatmydata:
  - https://github.com/stewartsmith/libeatmydata/
  - https://github.com/stewartsmith/libeatmydata/blob/master/libeatmydata/libeatmydata.c