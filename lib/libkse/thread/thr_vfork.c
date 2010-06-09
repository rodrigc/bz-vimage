/*
 * $FreeBSD: src/lib/libkse/thread/thr_vfork.c,v 1.7 2007/12/16 23:29:57 deischen Exp $
 */

#include <unistd.h>
#include "thr_private.h"

int _vfork(void);

__weak_reference(_vfork, vfork);

int
_vfork(void)
{
	return (fork());
}
