#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93
# $FreeBSD: src/lib/libc/i386/string/Makefile.inc,v 1.14 2010/10/01 13:10:11 jilles Exp $

MDSRCS+=bcmp.S bcopy.S bzero.S ffs.S index.S memchr.S memcmp.S memcpy.S \
	memmove.S memset.S rindex.S strcat.S strchr.S strcmp.S strcpy.S \
	strncmp.S strrchr.S swab.S wcschr.S wcscmp.S wcslen.S \
	wmemchr.S
