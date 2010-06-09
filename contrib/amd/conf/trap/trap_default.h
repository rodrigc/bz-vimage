/* $FreeBSD: src/contrib/amd/conf/trap/trap_default.h,v 1.4 2007/12/05 16:57:04 obrien Exp $ */
/* $srcdir/conf/trap/trap_default.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) mount(type, mnt->mnt_dir, flags, mnt_data)
