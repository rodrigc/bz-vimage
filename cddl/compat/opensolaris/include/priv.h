/* $FreeBSD: src/cddl/compat/opensolaris/include/priv.h,v 1.3 2008/04/22 07:42:58 jb Exp $ */

#ifndef	_OPENSOLARIS_PRIV_H_
#define	_OPENSOLARIS_PRIV_H_

#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#define	PRIV_SYS_CONFIG	0

static __inline int
priv_ineffect(priv)
{

	assert(priv == PRIV_SYS_CONFIG);
	return (geteuid() == 0);
}

#endif	/* !_OPENSOLARIS_PRIV_H_ */
