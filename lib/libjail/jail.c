/*-
 * Copyright (c) 2009 James Gritton.
 * Copyright (c) 2010 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by CK Software GmbH
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libjail/jail.c,v 1.6 2010/07/15 19:21:07 jamie Exp $");

#define _WANT_CPUSET
#define _WANT_PRISON
#define _WANT_UCRED
#include <sys/param.h>
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/_task.h>
#include <sys/jail.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/queue.h>
#include <sys/cpuset.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jail.h"

#define	SJPARAM		"security.jail.param"

#define JPS_IN_ADDR	1
#define JPS_IN6_ADDR	2

#define ARRAY_SANITY	5
#define ARRAY_SLOP	5


static int jailparam_import_enum(const char **values, int nvalues,
    const char *valstr, size_t valsize, int *value);
static int jailparam_type(struct jailparam *jp);
static char *noname(const char *name);
static char *nononame(const char *name);

char jail_errmsg[JAIL_ERRMSGLEN];

static const char *bool_values[] = { "false", "true" };
static const char *jailsys_values[] = { "disable", "new", "inherit" };


/*
 * Import a null-terminated parameter list and set a jail with the flags
 * and parameters.
 */
int
jail_setv(int flags, ...)
{
	va_list ap, tap;
	struct jailparam *jp;
	const char *name, *value;
	int njp, jid;

	/* Create the parameter list and import the parameters. */
	va_start(ap, flags);
	va_copy(tap, ap);
	for (njp = 0; va_arg(tap, char *) != NULL; njp++)
		(void)va_arg(tap, char *);
	va_end(tap);
	jp = alloca(njp * sizeof(struct jailparam));
	for (njp = 0; (name = va_arg(ap, char *)) != NULL; njp++) {
		value = va_arg(ap, char *);
		if (jailparam_init(jp + njp, name) < 0 ||
		    jailparam_import(jp + njp, value) < 0) {
			jailparam_free(jp, njp);
			va_end(ap);
			return (-1);
		}
	}
	va_end(ap);
	jid = jailparam_set(jp, njp, flags);
	jailparam_free(jp, njp);
	return (jid);
}

/*
 * Read a null-terminated parameter list, get the referenced jail, and export
 * the parameters to the list.
 */
int
jail_getv(int flags, ...)
{
	va_list ap, tap;
	struct jailparam *jp, *jp_lastjid, *jp_jid, *jp_name, *jp_key;
	char *valarg, *value;
	const char *name, *key_value, *lastjid_value, *jid_value, *name_value;
	int njp, i, jid;

	/* Create the parameter list and find the key. */
	va_start(ap, flags);
	va_copy(tap, ap);
	for (njp = 0; va_arg(tap, char *) != NULL; njp++)
		(void)va_arg(tap, char *);
	va_end(tap);

	jp = alloca(njp * sizeof(struct jailparam));
	va_copy(tap, ap);
	jp_lastjid = jp_jid = jp_name = NULL;
	lastjid_value = jid_value = name_value = NULL;
	for (njp = 0; (name = va_arg(tap, char *)) != NULL; njp++) {
		value = va_arg(tap, char *);
		if (jailparam_init(jp + njp, name) < 0) {
			va_end(tap);
			goto error;
		}
		if (!strcmp(jp[njp].jp_name, "lastjid")) {
			jp_lastjid = jp + njp;
			lastjid_value = value;
		} else if (!strcmp(jp[njp].jp_name, "jid")) {
			jp_jid = jp + njp;
			jid_value = value;
		} if (!strcmp(jp[njp].jp_name, "name")) {
			jp_name = jp + njp;
			name_value = value;
		}
	}
	va_end(tap);
	/* Import the key parameter. */
	if (jp_lastjid != NULL) {
		jp_key = jp_lastjid;
		key_value = lastjid_value;
	} else if (jp_jid != NULL && strtol(jid_value, NULL, 10) != 0) {
		jp_key = jp_jid;
		key_value = jid_value;
	} else if (jp_name != NULL) {
		jp_key = jp_name;
		key_value = name_value;
	} else {
		strlcpy(jail_errmsg, "no jail specified", JAIL_ERRMSGLEN);
		errno = ENOENT;
		goto error;
	}
	if (jailparam_import(jp_key, key_value) < 0)
		goto error;
	/* Get the jail and export the parameters. */
	jid = jailparam_get(jp, njp, flags);
	if (jid < 0)
		goto error;
	for (i = 0; i < njp; i++) {
		(void)va_arg(ap, char *);
		valarg = va_arg(ap, char *);
		if (jp + i != jp_key) {
			/* It's up to the caller to ensure there's room. */
			if ((jp[i].jp_ctltype & CTLTYPE) == CTLTYPE_STRING)
				strcpy(valarg, jp[i].jp_value);
			else {
				value = jailparam_export(jp + i);
				if (value == NULL)
					goto error;
				strcpy(valarg, value);
				free(value);
			}
		}
	}
	jailparam_free(jp, njp);
	va_end(ap);
	return (jid);

 error:
	jailparam_free(jp, njp);
	va_end(ap);
	return (-1);
}

/*
 * Return a list of all known parameters.
 */
int
jailparam_all(struct jailparam **jpp)
{
	struct jailparam *jp;
	size_t mlen1, mlen2, buflen;
	int njp, count;
	int mib1[CTL_MAXNAME], mib2[CTL_MAXNAME - 2];
	char buf[MAXPATHLEN];

	njp = 0;
	count = 32;
	jp = malloc(count * sizeof(*jp));
	if (jp == NULL) {
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (-1);
	}
	mib1[0] = 0;
	mib1[1] = 2;
	mlen1 = CTL_MAXNAME - 2;
	if (sysctlnametomib(SJPARAM, mib1 + 2, &mlen1) < 0) {
		snprintf(jail_errmsg, JAIL_ERRMSGLEN,
		    "sysctlnametomib(" SJPARAM "): %s", strerror(errno));
		goto error;
	}
	for (;; njp++) {
		/* Get the next parameter. */
		mlen2 = sizeof(mib2);
		if (sysctl(mib1, mlen1 + 2, mib2, &mlen2, NULL, 0) < 0) {
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "sysctl(0.2): %s", strerror(errno));
			goto error;
		}
		if (mib2[0] != mib1[2] || mib2[1] != mib1[3] ||
		    mib2[2] != mib1[4])
			break;
		/* Convert it to an ascii name. */
		memcpy(mib1 + 2, mib2, mlen2);
		mlen1 = mlen2 / sizeof(int);
		mib1[1] = 1;
		buflen = sizeof(buf);
		if (sysctl(mib1, mlen1 + 2, buf, &buflen, NULL, 0) < 0) {
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "sysctl(0.1): %s", strerror(errno));
			goto error;
		}
		if (buf[buflen - 2] == '.')
			buf[buflen - 2] = '\0';
		/* Add the parameter to the list */
		if (njp >= count) {
			count *= 2;
			jp = realloc(jp, count * sizeof(*jp));
			if (jp == NULL) {
				jailparam_free(jp, njp);
				return (-1);
			}
		}
		if (jailparam_init(jp + njp, buf + sizeof(SJPARAM)) < 0)
			goto error;
		if (jailparam_type(jp + njp) < 0) {
			njp++;
			goto error;
		}
		mib1[1] = 2;
	}
	jp = realloc(jp, njp * sizeof(*jp));
	*jpp = jp;
	return (njp);

 error:
	jailparam_free(jp, njp);
	free(jp);
	return (-1);
}

/*
 * Clear a jail parameter and copy in its name.
 */
int
jailparam_init(struct jailparam *jp, const char *name)
{

	memset(jp, 0, sizeof(*jp));
	jp->jp_name = strdup(name);
	if (jp->jp_name == NULL) {
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (-1);
	}
	return (0);
}

/*
 * Put a name and value into a jail parameter element, converting the value
 * to internal form.
 */
int
jailparam_import(struct jailparam *jp, const char *value)
{
	char *p, *ep, *tvalue;
	const char *avalue;
	int i, nval, fw;

	if (!jp->jp_ctltype && jailparam_type(jp) < 0)
		return (-1);
	if (value == NULL)
		return (0);
	if ((jp->jp_ctltype & CTLTYPE) == CTLTYPE_STRING) {
		jp->jp_value = strdup(value);
		if (jp->jp_value == NULL) {
			strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
			return (-1);
		}
		return (0);
	}
	nval = 1;
	if (jp->jp_elemlen) {
		if (value[0] == '\0' || (value[0] == '-' && value[1] == '\0')) {
			jp->jp_value = strdup("");
			if (jp->jp_value == NULL) {
				strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
				return (-1);
			}
			jp->jp_valuelen = 0;
			return (0);
		}
		for (p = strchr(value, ','); p; p = strchr(p + 1, ','))
			nval++;
		jp->jp_valuelen = jp->jp_elemlen * nval;
	}
	jp->jp_value = malloc(jp->jp_valuelen);
	if (jp->jp_value == NULL) {
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (-1);
	}
	avalue = value;
	for (i = 0; i < nval; i++) {
		fw = nval == 1 ? strlen(avalue) : strcspn(avalue, ",");
		switch (jp->jp_ctltype & CTLTYPE) {
		case CTLTYPE_INT:
			if (jp->jp_flags & (JP_BOOL | JP_NOBOOL)) {
				if (!jailparam_import_enum(bool_values, 2,
				    avalue, fw, &((int *)jp->jp_value)[i])) {
					snprintf(jail_errmsg,
					    JAIL_ERRMSGLEN, "%s: "
					    "unknown boolean value \"%.*s\"",
					    jp->jp_name, fw, avalue);
					errno = EINVAL;
					goto error;
				}
				break;
			}
			if (jp->jp_flags & JP_JAILSYS) {
				/*
				 * Allow setting a jailsys parameter to "new"
				 * in a booleanesque fashion.
				 */
				if (value[0] == '\0')
					((int *)jp->jp_value)[i] = JAIL_SYS_NEW;
				else if (!jailparam_import_enum(jailsys_values,
				    sizeof(jailsys_values) /
				    sizeof(jailsys_values[0]), avalue, fw,
				    &((int *)jp->jp_value)[i])) {
					snprintf(jail_errmsg,
					    JAIL_ERRMSGLEN, "%s: "
					    "unknown jailsys value \"%.*s\"",
					    jp->jp_name, fw, avalue);
					errno = EINVAL;
					goto error;
				}
				break;
			}
			((int *)jp->jp_value)[i] = strtol(avalue, &ep, 10);
		integer_test:
			if (ep != avalue + fw) {
				snprintf(jail_errmsg, JAIL_ERRMSGLEN,
				    "%s: non-integer value \"%.*s\"",
				    jp->jp_name, fw, avalue);
				errno = EINVAL;
				goto error;
			}
			break;
		case CTLTYPE_UINT:
			((unsigned *)jp->jp_value)[i] =
			    strtoul(avalue, &ep, 10);
			goto integer_test;
		case CTLTYPE_LONG:
			((long *)jp->jp_value)[i] = strtol(avalue, &ep, 10);
			goto integer_test;
		case CTLTYPE_ULONG:
			((unsigned long *)jp->jp_value)[i] =
			    strtoul(avalue, &ep, 10);
			goto integer_test;
		case CTLTYPE_QUAD:
			((int64_t *)jp->jp_value)[i] =
			    strtoimax(avalue, &ep, 10);
			goto integer_test;
		case CTLTYPE_STRUCT:
			tvalue = alloca(fw + 1);
			strlcpy(tvalue, avalue, fw + 1);
			switch (jp->jp_structtype) {
			case JPS_IN_ADDR:
				if (inet_pton(AF_INET, tvalue,
				    &((struct in_addr *)jp->jp_value)[i]) != 1)
				{
					snprintf(jail_errmsg,
					    JAIL_ERRMSGLEN,
					    "%s: not an IPv4 address: %s",
					    jp->jp_name, tvalue);
					errno = EINVAL;
					goto error;
				}
				break;
			case JPS_IN6_ADDR:
				if (inet_pton(AF_INET6, tvalue,
				    &((struct in6_addr *)jp->jp_value)[i]) != 1)
				{
					snprintf(jail_errmsg,
					    JAIL_ERRMSGLEN,
					    "%s: not an IPv6 address: %s",
					    jp->jp_name, tvalue);
					errno = EINVAL;
					goto error;
				}
				break;
			default:
				goto unknown_type;
			}
			break;
		default:
		unknown_type:
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "unknown type for %s", jp->jp_name);
			errno = ENOENT;
			goto error;
		}
		avalue += fw + 1;
	}
	return (0);

 error:
	free(jp->jp_value);
	jp->jp_value = NULL;
	return (-1);
}

static int
jailparam_import_enum(const char **values, int nvalues, const char *valstr,
    size_t valsize, int *value)
{
	char *ep;
	int i;

	for (i = 0; i < nvalues; i++)
		if (valsize == strlen(values[i]) &&
		    !strncasecmp(valstr, values[i], valsize)) {
			*value = i;
			return 1;
		}
	*value = strtol(valstr, &ep, 10);
	return (ep == valstr + valsize);
}

/*
 * Put a name and value into a jail parameter element, copying the value
 * but not altering it.
 */
int
jailparam_import_raw(struct jailparam *jp, void *value, size_t valuelen)
{

	jp->jp_value = value;
	jp->jp_valuelen = valuelen;
	jp->jp_flags |= JP_RAWVALUE;
	return (0);
}

/*
 * Run the jail_set and jail_get system calls on a parameter list.
 */
int
jailparam_set(struct jailparam *jp, unsigned njp, int flags)
{
	struct iovec *jiov;
	char *nname;
	int i, jid, bool0;
	unsigned j;

	jiov = alloca(sizeof(struct iovec) * 2 * (njp + 1));
	bool0 = 0;
	for (i = j = 0; j < njp; j++) {
		jiov[i].iov_base = jp[j].jp_name;
		jiov[i].iov_len = strlen(jp[j].jp_name) + 1;
		i++;
		if (jp[j].jp_flags & (JP_BOOL | JP_NOBOOL)) {
			/*
			 * Set booleans without values.  If one has a value of
			 * zero, change it to (or from) its "no" counterpart.
			 */
			jiov[i].iov_base = NULL;
			jiov[i].iov_len = 0;
			if (jp[j].jp_value != NULL &&
			    jp[j].jp_valuelen == sizeof(int) &&
			    !*(int *)jp[j].jp_value) {
				bool0 = 1;
				nname = jp[j].jp_flags & JP_BOOL
				    ? noname(jp[j].jp_name)
				    : nononame(jp[j].jp_name);
				if (nname == NULL) {
					njp = j;
					jid = -1;
					goto done;
				}
				jiov[i - 1].iov_base = nname;
				jiov[i - 1].iov_len = strlen(nname) + 1;
				
			}
		} else {
			/*
			 * Try to fill in missing values with an empty string.
			 */
			if (jp[j].jp_value == NULL && jp[j].jp_valuelen > 0 &&
			    jailparam_import(jp + j, "") < 0) {
				njp = j;
				jid = -1;
				goto done;
			}
			jiov[i].iov_base = jp[j].jp_value;
			jiov[i].iov_len =
			    (jp[j].jp_ctltype & CTLTYPE) == CTLTYPE_STRING
			    ? strlen(jp[j].jp_value) + 1
			    : jp[j].jp_valuelen;
		}
		i++;
	}
	*(const void **)&jiov[i].iov_base = "errmsg";
	jiov[i].iov_len = sizeof("errmsg");
	i++;
	jiov[i].iov_base = jail_errmsg;
	jiov[i].iov_len = JAIL_ERRMSGLEN;
	i++;
	jail_errmsg[0] = 0;
	jid = jail_set(jiov, i, flags);
	if (jid < 0 && !jail_errmsg[0])
		snprintf(jail_errmsg, sizeof(jail_errmsg), "jail_set: %s",
		    strerror(errno));
 done:
	if (bool0)
		for (j = 0; j < njp; j++)
			if ((jp[j].jp_flags & (JP_BOOL | JP_NOBOOL)) &&
			    jp[j].jp_value != NULL &&
			    jp[j].jp_valuelen == sizeof(int) &&
			    !*(int *)jp[j].jp_value)
				free(jiov[j * 2].iov_base);
	return (jid);
}

TAILQ_HEAD(prisonlist, prison);
static int
_jail_get_copyopt(struct iovec *iovp, unsigned int iovcnt,
    const char *name, void *dest, size_t len)
{
	unsigned int i;

	for (i = 0; i < (iovcnt - 1); i++) {

		/* No name. Just return an error. */
		if (iovp[i].iov_base == NULL)
			break;
		if (iovp[i+1].iov_len != len ||
		    iovp[i+1].iov_base == NULL) {
			/* Value not matching or error, skip pair. */
			i++;
			continue;
		}
		/* This is the pair we are looking for. */
		if (!strcmp(name, iovp[i].iov_base)) {
			bcopy(iovp[i+1].iov_base, dest, iovp[i+1].iov_len);
			return (0);
		}
	}
	return (ENOENT);
}

static int
_jail_get_getopt(struct iovec *iovp, unsigned int iovcnt,
    const char *name, char **buf, int *len)
{
	unsigned int i;

	for (i = 0; i < (iovcnt - 1); i++) {
		/* No name. Just return an error. */
		if (iovp[i].iov_base == NULL)
			break;
		/* This is the pair we are looking for. */
		if (!strcmp(name, iovp[i].iov_base)) {
			if (len != NULL)
				*len = iovp[i+1].iov_len;
			if (buf != NULL)
				*buf = (char *)iovp[i+1].iov_base;
			return (0);
		}
		/* Skip pair. */
		i++;
	}
	return (ENOENT);
}

static int
_jail_get_setopt(struct iovec *iovp, unsigned int iovcnt,
    const char *name, void *value, size_t len)
{
	unsigned int i;

	for (i = 0; i < (iovcnt - 1); i++) {
		/* No name. Just return an error. */
		if (iovp[i].iov_base == NULL)
			break;
		/* Is this the pair we are looking for? */
		if (strcmp(name, iovp[i].iov_base)) {
			i++;
			continue;
		}
		/* Skip name and work on value iovec. */
		i++;
		if (iovp[i].iov_base == NULL) {
			iovp[i].iov_len = len;
		} else {
			if (iovp[i].iov_len != len)
				return (EINVAL);
			bcopy(value, iovp[i].iov_base, len);
		}
		return (0);
	}
	return (ENOENT);
}

static int
_jail_get_setopt_part(struct iovec *iovp, unsigned int iovcnt,
    const char *name, void *value, size_t len)
{
	unsigned int i;

	for (i = 0; i < (iovcnt - 1); i++) {
		/* No name. Just return an error. */
		if (iovp[i].iov_base == NULL)
			break;
		/* Is this the pair we are looking for? */
                if (strcmp(name, iovp[i].iov_base)) {
			i++;
                        continue;
		}
		/* Skip name and work on value iovec. */
                i++;
		if (iovp[i].iov_base == NULL)
			iovp[i].iov_len = len;
                else {
			if (iovp[i].iov_len < len)
				return (EINVAL);
			iovp[i].iov_len = len;
			bcopy(value, iovp[i].iov_base, len);
                }
                return (0);
        }
        return (ENOENT);
}

static int
_jail_get_setopts(struct iovec *iovp, unsigned int iovcnt,
    const char *name, const char *value)
{
	unsigned int i;

	for (i = 0; i < (iovcnt - 1); i++) {
		/* No name. Just return an error. */
		if (iovp[i].iov_base == NULL)
			break;
		/* Is this the pair we are looking for? */
                if (strcmp(name, iovp[i].iov_base)) {
			i++;
                        continue;
		}
		/* Skip name and work on value iovec. */
                i++;
		if (iovp[i].iov_base == NULL)
                        iovp[i].iov_len = strlen(value) + 1;
                else if (strlcpy(iovp[i].iov_base, value, iovp[i].iov_len) >=
		    iovp[i].iov_len)
                        return (EINVAL);
                return (0);
        }
        return (ENOENT);
}

static void
_jail_get_kvm_opterror(struct iovec *iovp, unsigned int iovcnt,
    const char *fmt, ...)
{
	va_list ap;
	int error, len;
	char *errmsg;

	error = _jail_get_getopt(iovp, iovcnt, "errmsg",
	    &errmsg, &len);
	if (error || errmsg == NULL || len <= 0)
		return;

	va_start(ap, fmt);
	vsnprintf(errmsg, (size_t)len, fmt, ap);
	va_end(ap);
}

static int
_jail_get_kvm_prison_read(kvm_t *kd, uintptr_t addr, void *buf, size_t size)
{
	ssize_t l;

	l = kvm_read(kd, addr, buf, size);
	if (l != (ssize_t)size) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s: %p", kvm_geterr(kd), (void *)addr);
		return (EINVAL);
	}
	return (0);
}

/* This is inefficient but correct. */
static char *
_jail_get_kvm_prison_read_string(kvm_t *kd, uintptr_t addr, char *buf, size_t size)
{
	size_t s;
	ssize_t l;

	for (s = 0; s < size; s++) {
		l = kvm_read(kd, addr, buf, s+1);
		if (l != (ssize_t)s+1)
			return (NULL);
		if (buf[s] == '\0')
			return buf;
	}

	return (NULL);
}

static int
_jail_get_kvm_prison_ischild(kvm_t *kd, struct prison *pr1,
    struct prison *pr2)
{
	uintptr_t praddr;
	struct prison pr;
	size_t size;

	size = sizeof(pr);
	praddr = (uintptr_t)pr2->pr_parent;
	while (praddr != 0) {
		if (_jail_get_kvm_prison_read(kd, (uintptr_t)praddr, &pr, size))
			return (0);
		if (pr1->pr_id == pr.pr_id)
			return (1);
		praddr = (uintptr_t)pr.pr_parent;
	}
	return (0);
}

static uintptr_t
_jail_get_kvm_prison_find_child(kvm_t *kd, uintptr_t mypraddr,
    struct prison *mypr, int prid)
{
	struct prison *cpr, pr, *rpr;
	int descend;
	size_t size;

	size = sizeof(pr);
	descend = 1;
	cpr = mypr;
	rpr = (struct prison *)mypraddr;
	while (cpr != NULL) {
		if (descend && cpr->pr_id == prid && cpr->pr_ref > 0)
			return ((uintptr_t)rpr);
		if (descend && !LIST_EMPTY(&cpr->pr_children)) {
			cpr = LIST_FIRST(&cpr->pr_children);
			if (_jail_get_kvm_prison_read(kd,
			    (uintptr_t)cpr, &pr, size))
				return (0);
			rpr = cpr;
			cpr = &pr;
		} else {
			if (cpr->pr_id == mypr->pr_id) {
				cpr = NULL;
				rpr = cpr;
			} else {
				if ((descend =
				    (LIST_NEXT(cpr, pr_sibling) != NULL))) {
					cpr = LIST_NEXT(cpr, pr_sibling);
					if (_jail_get_kvm_prison_read(kd,
					    (uintptr_t)cpr, &pr, size))
						return (0);
					rpr = cpr;
					cpr = &pr;
				} else {
					cpr = cpr->pr_parent;
					if (_jail_get_kvm_prison_read(kd,
					    (uintptr_t)cpr, &pr, size))
						return (0);
					rpr = cpr;
					cpr = &pr;
				}
				
			}
		}
         }
         return (0);
}

static const char *
_jail_get_kvm_prison_name(kvm_t *kd, struct prison *pr1, struct prison *pr2)
{
	const char *name;
	struct prison pr;

	/* Jails see themselves as "0" (if they see themselves at all). */
	if (pr1->pr_id == pr2->pr_id)
		return ("0");

	name = pr2->pr_name;
	if (_jail_get_kvm_prison_ischild(kd, pr1, pr2)) {
		/*
		 * In kernel we do not neccessarily hold a lock,
		 * but the number of dots can be counted on - and counted.
		 */
		pr = *pr1;
		while (pr.pr_id != /* prison0 */ 0) {
			name = strchr(name, '.') + 1;
			if (pr.pr_parent == NULL)
				break;
			if (_jail_get_kvm_prison_read(kd,
			    (uintptr_t)pr.pr_parent, &pr, sizeof(pr)))
				return (NULL);
		}
	}
	return (name);
}

static const char *
_jail_get_kvm_prison_path(kvm_t *kd __unused, struct prison *pr1,
    struct prison *pr2)
{
	const char *path1, *path2;
	int len1;

	path1 = pr1->pr_path;
	path2 = pr2->pr_path;
	if (!strcmp(path1, "/"))
		return (path2);
	len1 = strlen(path1);
	if (strncmp(path1, path2, len1))
		return (path2);
	if (path2[len1] == '\0')
		return "/";
	if (path2[len1] == '/')
		return (path2 + len1);
	return (path2);
}

static uintptr_t
_jail_get_kvm_prison_find_name(kvm_t *kd, uintptr_t mypraddr,
    struct prison *mypr, const char *name)
{
	struct prison *cpr, pr, *deadpr, *rpr, *deadrpr;
	size_t mylen, size;
	int descend;

	mylen = (mypr->pr_id == /*prison0*/0) ? 0 : strlen(mypr->pr_name) + 1;
	size = sizeof(pr);
again:
	deadpr = NULL;
	deadrpr = NULL;
	descend = 1;
	rpr = (struct prison *)mypraddr;
	cpr = mypr;
	while (cpr != NULL) {
		if (descend && !strcmp(cpr->pr_name + mylen, name)) {
			if (cpr->pr_ref > 0) {
				if (cpr->pr_uref > 0)
					return ((uintptr_t)rpr);
				deadrpr = rpr;
				deadpr = cpr;
			}
		}
		if (descend && !LIST_EMPTY(&cpr->pr_children)) {
			cpr = LIST_FIRST(&cpr->pr_children);
			if (_jail_get_kvm_prison_read(kd,
			    (uintptr_t)cpr, &pr, size))
				return (0);
			rpr = cpr;
			cpr = &pr;
		} else {
			if (cpr->pr_id == mypr->pr_id) {
				cpr = NULL;
				rpr = cpr;
			} else {
				if ((descend =
				    (LIST_NEXT(cpr, pr_sibling) != NULL))) {
					cpr = LIST_NEXT(cpr, pr_sibling);
					if (_jail_get_kvm_prison_read(kd,
					    (uintptr_t)cpr, &pr, size))
						return (0);
					rpr = cpr;
					cpr = &pr;
				} else {
					cpr = cpr->pr_parent;
					if (_jail_get_kvm_prison_read(kd,
					    (uintptr_t)cpr, &pr, size))
						return (0);
					rpr = cpr;
					cpr = &pr;
				}
				
			}
		}
	}
	/* There was no valid prison - perhaps there was a dying one. */
	if (deadpr != NULL && deadpr->pr_ref == 0)
		goto again;
	return ((uintptr_t)deadrpr);
}

static int
_jail_get_kvm(struct iovec *iovp, unsigned int iovcnt, int flags,
    char *nlistf, char *memf)
{
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	size_t size;
	struct prisonlist allprison;
	struct prison pr, mypr, pr2, *prp;
	uintptr_t praddr;
	char *name, *p;
	int error, jid, len, i;
	uintptr_t procp, credp;
	lwpid_t dumptid;
	pid_t pid;
	struct proc proc;
	struct ucred cred;
	struct cpuset cpuset;
	size_t pr_flag_names_size;
	size_t pr_flag_nonames_size;
	size_t pr_flag_jailsys_size;
	size_t pr_allow_names_size;
	size_t pr_allow_nonames_size;
	char **pr_allow_names;
	char **pr_allow_nonames;
	struct jailsys_flags {
		 const char	*name;
		 unsigned	 disable;
		 unsigned	 new;
	} *pr_flag_jailsys;
	char **pr_flag_names;
	char **pr_flag_nonames;
	char buf[256];
	size_t fi;
	unsigned u;

	static struct nlist nl[] = {
#define	NLIST_ALLPRISON			0
		{ .n_name = "_allprison" },
#define NLIST_ALLPROC			1
		{ .n_name = "allproc" },
#define NLIST_DUMPTID			2
		{ .n_name = "dumptid" },
#define NLIST_PROC0			3
		{ .n_name = "proc0" },
#define NLIST_PR_FLAG_NAMES		4
		{ .n_name = "pr_flag_names" },
#define NLIST_PR_FLAG_NAMES_SIZE	5
		{ .n_name = "pr_flag_names_size" },
#define NLIST_PR_FLAG_NONAMES		6
		{ .n_name = "pr_flag_nonames" },
#define NLIST_PR_FLAG_NONAMES_SIZE	7
		{ .n_name = "pr_flag_nonames_size" },
#define NLIST_PR_FLAG_JAILSYS		8
		{ .n_name = "pr_flag_jailsys" },
#define NLIST_PR_FLAG_JAILSYS_SIZE	9
		{ .n_name = "pr_flag_jailsys_size" },
#define NLIST_PR_ALLOW_NAMES		10
		{ .n_name = "pr_allow_names" },
#define NLIST_PR_ALLOW_NAMES_SIZE	11
		{ .n_name = "pr_allow_names_size" },
#define NLIST_PR_ALLOW_NONAMES		12
		{ .n_name = "pr_allow_nonames" },
#define NLIST_PR_ALLOW_NONAMES_SIZE	13
		{ .n_name = "pr_allow_nonames_size" },
		{ .n_name = NULL },
	};

	if (flags & ~JAIL_GET_MASK) {
		errno = EINVAL;
		goto err;
	}

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);
	if (kd != NULL) {
		if (kvm_nlist(kd, nl) < 0) {
			if (nlistf) {
				if (!jail_errmsg[0])
					snprintf(jail_errmsg, sizeof(jail_errmsg),
					    "%s: kvm_nlist: %s", nlistf,
					    kvm_geterr(kd));
			} else {
				if (!jail_errmsg[0])
					snprintf(jail_errmsg, sizeof(jail_errmsg),
					    "kvm_nlist: %s", kvm_geterr(kd));
			}
			errno = EINVAL;
			goto err;
		}

		if (nl[0].n_type == 0) {
			if (nlistf) {
				if (!jail_errmsg[0])
					snprintf(jail_errmsg, sizeof(jail_errmsg),
					    "%s: no namelist", nlistf);
			} else {
				if (!jail_errmsg[0])
					snprintf(jail_errmsg, sizeof(jail_errmsg),
					    "no namelist");
			}
			errno = EINVAL;
			goto err;
		}
	} else {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "available: %s", errbuf);
		errno = EINVAL;
		goto err;
	}

	/* Get allprison list head. */
	size = sizeof(allprison);
	if (kvm_read(kd, nl[NLIST_ALLPRISON].n_value, &allprison, size) !=
	    (ssize_t)size) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s", kvm_geterr(kd));
		errno = EINVAL;
		goto err;
	}

	/*
	 * Identify the process of this prison or auto-detect if this is
	 * a crashdump by reading dumptid.
	 */
	pid = getpid();
	dumptid = 0;
	if (nl[NLIST_DUMPTID].n_value) {
		if (kvm_read(kd, nl[NLIST_DUMPTID].n_value, &dumptid,
		    sizeof(dumptid)) != sizeof(dumptid)) {
			if (!jail_errmsg[0])
				snprintf(jail_errmsg, sizeof(jail_errmsg),
				    "%s: cannot read dumptid", __func__);
			errno = EINVAL;
			goto err;
		}
	}

	/*
	 * First, find the process for this pid.  If we are working on a dump,
	 * locate proc0.  Based on either, take the address of the ucred.
	 */
	credp = 0;
	procp = nl[NLIST_ALLPROC].n_value;
	if (dumptid > 0) {
		procp = nl[NLIST_PROC0].n_value;
		pid = 0;
	}

	while (procp != 0) {
		/* For a live system this is best effort. */
		if (kvm_read(kd, procp, &proc, sizeof(proc)) != sizeof(proc)) {
			if (!jail_errmsg[0])
				snprintf(jail_errmsg, sizeof(jail_errmsg),
				    "%s: cannot read proc", __func__);
			errno = EINVAL;
			goto err;
		}
		if (proc.p_pid == pid)
			credp = (uintptr_t)proc.p_ucred;
		if (credp != 0)
			break;
		procp = (uintptr_t)LIST_NEXT(&proc, p_list);
	}
	if (credp == 0) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s: cannot identify cred", __func__);
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, (uintptr_t)credp, &cred, sizeof(cred)) !=
	    sizeof(cred)) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s: cannot read cred", __func__);
		errno = EINVAL;
		goto err;
	}
	if (cred.cr_prison == NULL) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s: cannot indentify prison", __func__);
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, (uintptr_t)cred.cr_prison, &mypr, sizeof(mypr)) !=
	    sizeof(mypr)) {
		if (!jail_errmsg[0])
			snprintf(jail_errmsg, sizeof(jail_errmsg),
			    "%s: cannot read prison", __func__);
		errno = EINVAL;
		goto err;
	}

	/*
	 * Find the prison specified by one of: lastjid, jid, name.
	 */
	error = _jail_get_copyopt(iovp, iovcnt, "lastjid", &jid, sizeof(jid));
	if (error == 0) {
		size = sizeof(pr);
		praddr = (uintptr_t)TAILQ_FIRST(&allprison);
		while (praddr != 0) {
			if (_jail_get_kvm_prison_read(kd, praddr, &pr, size)) {
				errno = EINVAL;
				goto err;
			}
			if (pr.pr_id > jid &&
			    _jail_get_kvm_prison_ischild(kd, &mypr, &pr)) {
				if (pr.pr_ref > 0 &&
				    (pr.pr_uref > 0 || (flags & JAIL_DYING)))
					praddr = (uintptr_t)&pr;
					break;
			}
			praddr = (uintptr_t)TAILQ_NEXT(&pr, pr_list);
		}
		if (praddr != 0)
			goto found_prison;
		_jail_get_kvm_opterror(iovp, iovcnt, "no jail after %d", jid);
		errno = ENOENT;
		goto err;
	} else if (error != ENOENT) {
		errno = EINVAL;
		goto err;
	}

	error = _jail_get_copyopt(iovp, iovcnt, "jid", &jid, sizeof(jid));
	if (error == 0) {
		if (jid != 0) {
			praddr = _jail_get_kvm_prison_find_child(kd,
			    (uintptr_t)cred.cr_prison, &mypr, jid);
			if (praddr != 0) {
				if (_jail_get_kvm_prison_read(kd, praddr, &pr,
				    sizeof(pr))) {
					errno = EINVAL;
					goto err;
				}
				if (pr.pr_uref == 0 && !(flags & JAIL_DYING)) {
					_jail_get_kvm_opterror(iovp, iovcnt,
					    "jail %d is dying", jid);
					errno = ENOENT;
					goto err;
				}
				goto found_prison;
			}
			_jail_get_kvm_opterror(iovp, iovcnt,
			    "jail %d not found", jid);
			errno = ENOENT;
			goto err;
		}
	} else if (error != ENOENT) {
		errno = EINVAL;
		goto err;
	}

	error = _jail_get_getopt(iovp, iovcnt, "name", &name, &len);
	if (error == 0) {
		if (len == 0 || name[len - 1] != '\0') {
			errno = EINVAL;
			goto err;
		}
		praddr = _jail_get_kvm_prison_find_name(kd,
		    (uintptr_t)cred.cr_prison, &mypr, name);
		if (praddr != 0) {
			if (_jail_get_kvm_prison_read(kd, praddr, &pr,
			    sizeof(pr))) {
				errno = EINVAL;
				goto err;
			}
			if (pr.pr_uref == 0 && !(flags & JAIL_DYING)) {
				_jail_get_kvm_opterror(iovp, iovcnt,
				    "jail \"%s\" is dying", name);
				errno = ENOENT;
				goto err;
			}
			goto found_prison;
		}
		_jail_get_kvm_opterror(iovp, iovcnt, "jail \"%s\" not found",
		    name);
		errno = ENOENT;
		goto err;
	} else if (error != ENOENT) {
		errno = EINVAL;
		goto err;
	}

	_jail_get_kvm_opterror(iovp, iovcnt, "no jail specified");
	errno = ENOENT;
	goto err;

found_prison:
	prp = &pr;
	error = _jail_get_setopt(iovp, iovcnt, "jid", &prp->pr_id,
	    sizeof(prp->pr_id));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	if (!prp->pr_parent ||
	    _jail_get_kvm_prison_read(kd, (uintptr_t)prp->pr_parent,
	    &pr2, sizeof(pr2))) {
		errno = EINVAL;
		goto err;
	}
	i = (pr2.pr_id == mypr.pr_id) ? 0 : pr2.pr_id;
	error = _jail_get_setopt(iovp, iovcnt, "parent", &i, sizeof(i));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopts(iovp, iovcnt, "name",
	    _jail_get_kvm_prison_name(kd, &mypr, prp));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	if (_jail_get_kvm_prison_read(kd, (uintptr_t)prp->pr_cpuset,
	    &cpuset, sizeof(cpuset))) {
		errno = EINVAL;
		goto err;
	}
	error = _jail_get_setopt(iovp, iovcnt, "cpuset.id", &cpuset.cs_id,
	    sizeof(cpuset.cs_id));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopts(iovp, iovcnt, "path",
	    _jail_get_kvm_prison_path(kd, &mypr, prp));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
#ifdef INET
	size = prp->pr_ip4s * sizeof(*prp->pr_ip4);
	p = malloc(size);
	if (p == NULL) {
		errno = EINVAL;
		goto err;
	}
	if (_jail_get_kvm_prison_read(kd, (uintptr_t)prp->pr_ip4, p, size)) {
		free(p);
		errno = EINVAL;
		goto err;
	}
	error = _jail_get_setopt_part(iovp, iovcnt, "ip4.addr", p, size);
	if (error != 0 && error != ENOENT) {
		free(p);
		errno = EINVAL;
		goto err_opts;
	}
	free(p);
#endif
#ifdef INET6
	size = prp->pr_ip6s * sizeof(*prp->pr_ip6);
	p = malloc(size);
	if (p == NULL) {
		errno = EINVAL;
		goto err;
	}
	if (_jail_get_kvm_prison_read(kd, (uintptr_t)prp->pr_ip6, p, size)) {
		free(p);
		errno = EINVAL;
		goto err;
	}
	error = _jail_get_setopt_part(iovp, iovcnt, "ip6.addr", p, size);
	if (error != 0 && error != ENOENT) {
		free(p);
		errno = EINVAL;
		goto err_opts;
	}
	free(p);
#endif
	error = _jail_get_setopt(iovp, iovcnt, "securelevel",
	    &prp->pr_securelevel, sizeof(prp->pr_securelevel));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopt(iovp, iovcnt, "children.cur",
	    &prp->pr_childcount, sizeof(prp->pr_childcount));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopt(iovp, iovcnt, "children.max",
	    &prp->pr_childmax, sizeof(prp->pr_childmax));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopts(iovp, iovcnt, "host.hostname",
	    prp->pr_hostname);
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopts(iovp, iovcnt, "host.domainname",
	    prp->pr_domainname);
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopts(iovp, iovcnt, "host.hostuuid",
	    prp->pr_hostuuid);
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopt(iovp, iovcnt, "host.hostid",
	    &prp->pr_hostid, sizeof(prp->pr_hostid));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	error = _jail_get_setopt(iovp, iovcnt, "enforce_statfs",
	    &prp->pr_enforce_statfs, sizeof(prp->pr_enforce_statfs));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}

	size = sizeof(pr_flag_names_size);
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NAMES_SIZE].n_value,
	    &pr_flag_names_size, size) != (ssize_t)size) {
		errno = EINVAL;
		goto err;
	}
	size = sizeof(pr_flag_nonames_size);
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NONAMES_SIZE].n_value,
	    &pr_flag_nonames_size, size) != (ssize_t)size) {
		errno = EINVAL;
		goto err;
	}

	size = sizeof(char *) * pr_flag_names_size;
	pr_flag_names = malloc(size);
	if (pr_flag_names == NULL) {
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NAMES].n_value,
	    pr_flag_names, size) != (ssize_t)size) {
		free(pr_flag_names);
		errno = EINVAL;
		goto err;
	}
	size = sizeof(char *) * pr_flag_nonames_size;
	pr_flag_nonames = malloc(size);
	if (pr_flag_names == NULL) {
		free(pr_flag_names);
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NONAMES].n_value,
	    pr_flag_nonames, size) != (ssize_t)size) {
		free(pr_flag_names);
		free(pr_flag_nonames);
		errno = EINVAL;
		goto err;
	}

	for (fi = 0; fi < pr_flag_names_size / sizeof(char *); fi++) {
		if (pr_flag_names[fi] == NULL || pr_flag_nonames[fi] == NULL)
			continue;
		i = (prp->pr_flags & (1 << fi)) ? 1 : 0;
		name = _jail_get_kvm_prison_read_string(kd,
		    (uintptr_t)pr_flag_names[fi], buf, sizeof(buf));
		if (name == NULL) {
			free(pr_flag_names);
			free(pr_flag_nonames);
			errno = EINVAL;
			goto err;
		}
		error = _jail_get_setopt(iovp, iovcnt, name, &i, sizeof(i));
		if (error != 0 && error != ENOENT) {
			free(pr_flag_names);
			free(pr_flag_nonames);
			errno = EINVAL;
			goto err_opts;
		}
		i = !i;
		name = _jail_get_kvm_prison_read_string(kd,
		    (uintptr_t)pr_flag_nonames[fi], buf, sizeof(buf));
		if (name == NULL) {
			free(pr_flag_names);
			free(pr_flag_nonames);
			errno = EINVAL;
			goto err;
		}
		error = _jail_get_setopt(iovp, iovcnt, name, &i, sizeof(i));
		if (error != 0 && error != ENOENT) {
			free(pr_flag_names);
			free(pr_flag_nonames);
			errno = EINVAL;
			goto err_opts;
		}
	}

	free(pr_flag_names);
	free(pr_flag_nonames);

	size = sizeof(pr_flag_jailsys_size);
	if (kvm_read(kd, nl[NLIST_PR_FLAG_JAILSYS_SIZE].n_value,
	    &pr_flag_jailsys_size, size) != (ssize_t)size) {
		errno = EINVAL;
		goto err;
	}

	size = sizeof(*pr_flag_jailsys) * pr_flag_jailsys_size;
	pr_flag_jailsys = malloc(size);
	if (pr_flag_jailsys == NULL) {
		errno = EINVAL;
		goto err;
	}

	if (kvm_read(kd, nl[NLIST_PR_FLAG_JAILSYS].n_value,
	    pr_flag_jailsys, size) != (ssize_t)size) {
		free(pr_flag_jailsys);
		errno = EINVAL;
		goto err;
	}

	for (fi = 0; fi < pr_flag_jailsys_size / sizeof(*pr_flag_jailsys);
	    fi++) {
		u = prp->pr_flags &
		    (pr_flag_jailsys[fi].disable | pr_flag_jailsys[fi].new);
		u = pr_flag_jailsys[fi].disable &&
		    (u == pr_flag_jailsys[fi].disable) ? JAIL_SYS_DISABLE
		    : (u == pr_flag_jailsys[fi].new) ? JAIL_SYS_NEW
		    : JAIL_SYS_INHERIT;
		name = _jail_get_kvm_prison_read_string(kd,
		    (uintptr_t)pr_flag_jailsys[fi].name, buf, sizeof(buf));
		if (name == NULL) {
			free(pr_flag_jailsys);
			errno = EINVAL;
			goto err;
		}
		error = _jail_get_setopt(iovp, iovcnt, name, &i, sizeof(i));
		if (error != 0 && error != ENOENT) {
			free(pr_flag_jailsys);
			errno = EINVAL;
			goto err_opts;
		}
	}
	free(pr_flag_jailsys);

	size = sizeof(pr_allow_names_size);
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NAMES_SIZE].n_value,
	    &pr_allow_names_size, size) != (ssize_t)size) {
		errno = EINVAL;
		goto err;
	}
	size = sizeof(pr_allow_nonames_size);
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NONAMES_SIZE].n_value,
	    &pr_allow_nonames_size, size) != (ssize_t)size) {
		errno = EINVAL;
		goto err;
	}

	size = sizeof(char *) * pr_allow_names_size;
	pr_allow_names = malloc(size);
	if (pr_allow_names == NULL) {
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NAMES].n_value,
	    pr_allow_names, size) != (ssize_t)size) {
		free(pr_allow_names);
		errno = EINVAL;
		goto err;
	}
	size = sizeof(char *) * pr_allow_nonames_size;
	pr_allow_nonames = malloc(size);
	if (pr_allow_nonames == NULL) {
		free(pr_allow_names);
		errno = EINVAL;
		goto err;
	}
	if (kvm_read(kd, nl[NLIST_PR_FLAG_NONAMES].n_value,
	    pr_allow_nonames, size) != (ssize_t)size) {
		free(pr_allow_names);
		free(pr_allow_nonames);
		errno = EINVAL;
		goto err;
	}

	for (fi = 0; fi < pr_allow_names_size / sizeof(char *); fi++) {
		if (pr_allow_names[fi] == NULL || pr_allow_nonames[fi] == NULL)
			continue;
		i = (prp->pr_allow & (1 << fi)) ? 1 : 0;
		name = _jail_get_kvm_prison_read_string(kd,
		    (uintptr_t)pr_allow_names[fi], buf, sizeof(buf));
		if (name == NULL) {
			free(pr_allow_names);
			free(pr_allow_nonames);
			errno = EINVAL;
			goto err;
		}
		error = _jail_get_setopt(iovp, iovcnt, name, &i, sizeof(i));
		if (error != 0 && error != ENOENT) {
			free(pr_allow_names);
			free(pr_allow_nonames);
			errno = EINVAL;
			goto err_opts;
		}
		i = !i;
		name = _jail_get_kvm_prison_read_string(kd,
		    (uintptr_t)pr_allow_nonames[fi], buf, sizeof(buf));
		if (name == NULL) {
			free(pr_allow_names);
			free(pr_allow_nonames);
			errno = EINVAL;
			goto err;
		}
		error = _jail_get_setopt(iovp, iovcnt, name, &i, sizeof(i));
		if (error != 0 && error != ENOENT) {
			free(pr_allow_names);
			free(pr_allow_nonames);
			errno = EINVAL;
			goto err_opts;
		}
	}
	free(pr_allow_names);
	free(pr_allow_nonames);

	i = (prp->pr_uref == 0);
	error = _jail_get_setopt(iovp, iovcnt, "dying", &i, sizeof(i));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}
	i = !i;
	error = _jail_get_setopt(iovp, iovcnt, "nodying", &i, sizeof(i));
	if (error != 0 && error != ENOENT) {
		errno = EINVAL;
		goto err_opts;
	}

	errno = 0;
	return (prp->pr_id);

err_opts:
	_jail_get_kvm_opterror(iovp, iovcnt, "error processing options");
err:
	return (-1);
}

static int
_jailparam_get(struct jailparam *jp, unsigned njp, int flags,
    char *nlistf, char *memf)
{
	struct iovec *jiov;
	struct jailparam *jp_lastjid, *jp_jid, *jp_name, *jp_key;
	int i, ai, ki, jid, arrays, sanity, usesyscall;
	unsigned j;

	usesyscall = (nlistf == NULL && memf == NULL);

	/*
	 * Get the types for all parameters.
	 * Find the key and any array parameters.
	 */
	jiov = alloca(sizeof(struct iovec) * 2 * (njp + 1));
	jp_lastjid = jp_jid = jp_name = NULL;
	arrays = 0;
	for (ai = j = 0; j < njp; j++) {
		if (!jp[j].jp_ctltype && jailparam_type(jp + j) < 0)
			return (-1);
		if (!strcmp(jp[j].jp_name, "lastjid"))
			jp_lastjid = jp + j;
		else if (!strcmp(jp[j].jp_name, "jid"))
			jp_jid = jp + j;
		else if (!strcmp(jp[j].jp_name, "name"))
			jp_name = jp + j;
		else if (jp[j].jp_elemlen && !(jp[j].jp_flags & JP_RAWVALUE)) {
			arrays = 1;
			jiov[ai].iov_base = jp[j].jp_name;
			jiov[ai].iov_len = strlen(jp[j].jp_name) + 1;
			ai++;
			jiov[ai].iov_base = NULL;
			jiov[ai].iov_len = 0;
			ai++;
		}
	}
	jp_key = jp_lastjid ? jp_lastjid :
	    jp_jid && jp_jid->jp_valuelen == sizeof(int) &&
	    jp_jid->jp_value && *(int *)jp_jid->jp_value ? jp_jid : jp_name;
	if (jp_key == NULL || jp_key->jp_value == NULL) {
		strlcpy(jail_errmsg, "no jail specified", JAIL_ERRMSGLEN);
		errno = ENOENT;
		return (-1);
	}
	ki = ai;
	jiov[ki].iov_base = jp_key->jp_name;
	jiov[ki].iov_len = strlen(jp_key->jp_name) + 1;
	ki++;
	jiov[ki].iov_base = jp_key->jp_value;
	jiov[ki].iov_len = (jp_key->jp_ctltype & CTLTYPE) == CTLTYPE_STRING
	    ? strlen(jp_key->jp_value) + 1 : jp_key->jp_valuelen;
	ki++;
	*(const void **)&jiov[ki].iov_base = "errmsg";
	jiov[ki].iov_len = sizeof("errmsg");
	ki++;
	jiov[ki].iov_base = jail_errmsg;
	jiov[ki].iov_len = JAIL_ERRMSGLEN;
	ki++;
	jail_errmsg[0] = 0;
	if (usesyscall) {
		if (arrays && jail_get(jiov, ki, flags) < 0) {
			if (!jail_errmsg[0])
				snprintf(jail_errmsg, sizeof(jail_errmsg),
				    "jail_get: %s", strerror(errno));
			return (-1);
		}
	} else {
		if (arrays && _jail_get_kvm(jiov, ki, flags,
		    nlistf, memf) < 0) {
			if (!jail_errmsg[0])
				snprintf(jail_errmsg, sizeof(jail_errmsg),
				    "jail_get_kvm: %s", strerror(errno));
			return (-1);
		}
	}
	/* Allocate storage for all parameters. */
	for (ai = j = 0, i = ki; j < njp; j++) {
		if (jp[j].jp_elemlen && !(jp[j].jp_flags & JP_RAWVALUE)) {
			ai++;
			jiov[ai].iov_len += jp[j].jp_elemlen * ARRAY_SLOP;
			if (jp[j].jp_valuelen >= jiov[ai].iov_len)
				jiov[ai].iov_len = jp[j].jp_valuelen;
			else {
				jp[j].jp_valuelen = jiov[ai].iov_len;
				if (jp[j].jp_value != NULL)
					free(jp[j].jp_value);
				jp[j].jp_value = malloc(jp[j].jp_valuelen);
				if (jp[j].jp_value == NULL) {
					strerror_r(errno, jail_errmsg,
					    JAIL_ERRMSGLEN);
					return (-1);
				}
			}
			jiov[ai].iov_base = jp[j].jp_value;
			memset(jiov[ai].iov_base, 0, jiov[ai].iov_len);
			ai++;
		} else if (jp + j != jp_key) {
			jiov[i].iov_base = jp[j].jp_name;
			jiov[i].iov_len = strlen(jp[j].jp_name) + 1;
			i++;
			if (jp[j].jp_value == NULL &&
			    !(jp[j].jp_flags & JP_RAWVALUE)) {
				jp[j].jp_value = malloc(jp[j].jp_valuelen);
				if (jp[j].jp_value == NULL) {
					strerror_r(errno, jail_errmsg,
					    JAIL_ERRMSGLEN);
					return (-1);
				}
			}
			jiov[i].iov_base = jp[j].jp_value;
			jiov[i].iov_len = jp[j].jp_valuelen;
			memset(jiov[i].iov_base, 0, jiov[i].iov_len);
			i++;
		}
	}
	/*
	 * Get the prison.  If there are array elements, retry a few times
	 * in case their sizes changed from under us.
	 */
	for (sanity = 0;; sanity++) {
		if (usesyscall)
			jid = jail_get(jiov, i, flags);
		else
			jid = _jail_get_kvm(jiov, i, flags, nlistf, memf);
		if (jid >= 0 || !arrays || sanity == ARRAY_SANITY ||
		    errno != EINVAL || jail_errmsg[0] ||
		    (sanity && !usesyscall))
			break;
		for (ai = j = 0; j < njp; j++) {
			if (jp[j].jp_elemlen &&
			    !(jp[j].jp_flags & JP_RAWVALUE)) {
				ai++;
				jiov[ai].iov_base = NULL;
				jiov[ai].iov_len = 0;
				ai++;
			}
		}
		if (usesyscall) {
			if (jail_get(jiov, ki, flags) < 0)
				break;
		} else {
			if (_jail_get_kvm(jiov, ki, flags, nlistf, memf) < 0)
				break;
		}
		for (ai = j = 0; j < njp; j++) {
			if (jp[j].jp_elemlen &&
			    !(jp[j].jp_flags & JP_RAWVALUE)) {
				ai++;
				jiov[ai].iov_len +=
				    jp[j].jp_elemlen * ARRAY_SLOP;
				if (jp[j].jp_valuelen >= jiov[ai].iov_len)
					jiov[ai].iov_len = jp[j].jp_valuelen;
				else {
					jp[j].jp_valuelen = jiov[ai].iov_len;
					if (jp[j].jp_value != NULL)
						free(jp[j].jp_value);
					jp[j].jp_value =
					    malloc(jiov[ai].iov_len);
					if (jp[j].jp_value == NULL) {
						strerror_r(errno, jail_errmsg,
						    JAIL_ERRMSGLEN);
						return (-1);
					}
				}
				jiov[ai].iov_base = jp[j].jp_value;
				memset(jiov[ai].iov_base, 0, jiov[ai].iov_len);
				ai++;
			}
		}
	}
	if (jid < 0 && !jail_errmsg[0])
		snprintf(jail_errmsg, sizeof(jail_errmsg),
		    "%s: %s", (usesyscall) ? "jail_get" : "jail_get_kvm",
		    strerror(errno));
	for (ai = j = 0, i = ki; j < njp; j++) {
		if (jp[j].jp_elemlen && !(jp[j].jp_flags & JP_RAWVALUE)) {
			ai++;
			jp[j].jp_valuelen = jiov[ai].iov_len;
			ai++;
		} else if (jp + j != jp_key) {
			i++;
			jp[j].jp_valuelen = jiov[i].iov_len;
			i++;
		}
	}
	return (jid);
}

int
jailparam_get(struct jailparam *jp, unsigned njp, int flags)
{

	return (_jailparam_get(jp, njp, flags, NULL, NULL));
}

int
jailparam_get_kvm(struct jailparam *jp, unsigned njp, int flags,
    char *nlistf, char *memf)
{

	return (_jailparam_get(jp, njp, flags, nlistf, memf));
}

/*
 * Convert a jail parameter's value to external form.
 */
char *
jailparam_export(struct jailparam *jp)
{
	char *value, *tvalue, **values;
	size_t valuelen;
	int i, nval, ival;
	char valbuf[INET6_ADDRSTRLEN];

	if (!jp->jp_ctltype && jailparam_type(jp) < 0)
		return (NULL);
	if ((jp->jp_ctltype & CTLTYPE) == CTLTYPE_STRING) {
		value = strdup(jp->jp_value);
		if (value == NULL)
			strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (value);
	}
	nval = jp->jp_elemlen ? jp->jp_valuelen / jp->jp_elemlen : 1;
	if (nval == 0) {
		value = strdup("");
		if (value == NULL)
			strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (value);
	}
	values = alloca(nval * sizeof(char *));
	valuelen = 0;
	for (i = 0; i < nval; i++) {
		switch (jp->jp_ctltype & CTLTYPE) {
		case CTLTYPE_INT:
			ival = ((int *)jp->jp_value)[i];
			if ((jp->jp_flags & (JP_BOOL | JP_NOBOOL)) &&
			    (unsigned)ival < 2) {
				strlcpy(valbuf, bool_values[ival],
				    sizeof(valbuf));
				break;
			}
			if ((jp->jp_flags & JP_JAILSYS) &&
			    (unsigned)ival < sizeof(jailsys_values) /
			    sizeof(jailsys_values[0])) {
				strlcpy(valbuf, jailsys_values[ival],
				    sizeof(valbuf));
				break;
			}
			snprintf(valbuf, sizeof(valbuf), "%d", ival);
			break;
		case CTLTYPE_UINT:
			snprintf(valbuf, sizeof(valbuf), "%u",
			    ((unsigned *)jp->jp_value)[i]);
			break;
		case CTLTYPE_LONG:
			snprintf(valbuf, sizeof(valbuf), "%ld",
			    ((long *)jp->jp_value)[i]);
			break;
		case CTLTYPE_ULONG:
			snprintf(valbuf, sizeof(valbuf), "%lu",
			    ((unsigned long *)jp->jp_value)[i]);
			break;
		case CTLTYPE_QUAD:
			snprintf(valbuf, sizeof(valbuf), "%jd",
			    (intmax_t)((int64_t *)jp->jp_value)[i]);
			break;
		case CTLTYPE_STRUCT:
			switch (jp->jp_structtype) {
			case JPS_IN_ADDR:
				if (inet_ntop(AF_INET,
				    &((struct in_addr *)jp->jp_value)[i],
				    valbuf, sizeof(valbuf)) == NULL) {
					strerror_r(errno, jail_errmsg,
					    JAIL_ERRMSGLEN);

					return (NULL);
				}
				break;
			case JPS_IN6_ADDR:
				if (inet_ntop(AF_INET6,
				    &((struct in6_addr *)jp->jp_value)[i],
				    valbuf, sizeof(valbuf)) == NULL) {
					strerror_r(errno, jail_errmsg,
					    JAIL_ERRMSGLEN);

					return (NULL);
				}
				break;
			default:
				goto unknown_type;
			}
			break;
		default:
		unknown_type:
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "unknown type for %s", jp->jp_name);
			errno = ENOENT;
			return (NULL);
		}
		valuelen += strlen(valbuf) + 1;
		values[i] = alloca(valuelen);
		strcpy(values[i], valbuf);
	}
	value = malloc(valuelen + 1);
	if (value == NULL)
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
	else {
		tvalue = value;
		for (i = 0; i < nval; i++) {
			strcpy(tvalue, values[i]);
			if (i < nval - 1) {
				tvalue += strlen(values[i]);
				*tvalue++ = ',';
			}
		}
	}
	return (value);
}

/*
 * Free the contents of a jail parameter list (but not thst list itself).
 */
void
jailparam_free(struct jailparam *jp, unsigned njp)
{
	unsigned j;

	for (j = 0; j < njp; j++) {
		free(jp[j].jp_name);
		if (!(jp[j].jp_flags & JP_RAWVALUE))
			free(jp[j].jp_value);
	}
}

/*
 * Find a parameter's type and size from its MIB.
 */
static int
jailparam_type(struct jailparam *jp)
{
	char *p, *nname;
	size_t miblen, desclen;
	int isarray;
	struct {
	    int i;
	    char s[MAXPATHLEN];
	} desc;
	int mib[CTL_MAXNAME];

	/* The "lastjid" parameter isn't real. */
	if (!strcmp(jp->jp_name, "lastjid")) {
		jp->jp_valuelen = sizeof(int);
		jp->jp_ctltype = CTLTYPE_INT | CTLFLAG_WR;
		return (0);
	}

	/* Find the sysctl that describes the parameter. */
	mib[0] = 0;
	mib[1] = 3;
	snprintf(desc.s, sizeof(desc.s), SJPARAM ".%s", jp->jp_name);
	miblen = sizeof(mib) - 2 * sizeof(int);
	if (sysctl(mib, 2, mib + 2, &miblen, desc.s, strlen(desc.s)) < 0) {
		if (errno != ENOENT) {
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "sysctl(0.3.%s): %s", jp->jp_name, strerror(errno));
			return (-1);
		}
		/*
		 * The parameter probably doesn't exist.  But it might be
		 * the "no" counterpart to a boolean.
		 */
		nname = nononame(jp->jp_name);
		if (nname != NULL) {
			snprintf(desc.s, sizeof(desc.s), SJPARAM ".%s", nname);
			free(nname);
			miblen = sizeof(mib) - 2 * sizeof(int);
			if (sysctl(mib, 2, mib + 2, &miblen, desc.s,
			    strlen(desc.s)) >= 0) {
				mib[1] = 4;
				desclen = sizeof(desc);
				if (sysctl(mib, (miblen / sizeof(int)) + 2,
					   &desc, &desclen, NULL, 0) < 0) {
					snprintf(jail_errmsg,
					    JAIL_ERRMSGLEN,
					    "sysctl(0.4.%s): %s", desc.s,
					    strerror(errno));
					return (-1);
				}
				if ((desc.i & CTLTYPE) == CTLTYPE_INT &&
				    desc.s[0] == 'B') {
					jp->jp_ctltype = desc.i;
					jp->jp_flags |= JP_NOBOOL;
					jp->jp_valuelen = sizeof(int);
					return (0);
				}
			}
		}
	unknown_parameter:
		snprintf(jail_errmsg, JAIL_ERRMSGLEN,
		    "unknown parameter: %s", jp->jp_name);
		errno = ENOENT;
		return (-1);
	}
 mib_desc:
	mib[1] = 4;
	desclen = sizeof(desc);
	if (sysctl(mib, (miblen / sizeof(int)) + 2, &desc, &desclen,
	    NULL, 0) < 0) {
		snprintf(jail_errmsg, JAIL_ERRMSGLEN,
		    "sysctl(0.4.%s): %s", jp->jp_name, strerror(errno));
		return (-1);
	}
	/* See if this is an array type. */
	p = strchr(desc.s, '\0');
	isarray  = 0;
	if (p - 2 < desc.s || strcmp(p - 2, ",a"))
		isarray = 0;
	else {
		isarray = 1;
		p[-2] = 0;
	}
	/* Look for types we understand */
	jp->jp_ctltype = desc.i;
	switch (desc.i & CTLTYPE) {
	case CTLTYPE_INT:
		if (desc.s[0] == 'B')
			jp->jp_flags |= JP_BOOL;
		else if (!strcmp(desc.s, "E,jailsys"))
			jp->jp_flags |= JP_JAILSYS;
	case CTLTYPE_UINT:
		jp->jp_valuelen = sizeof(int);
		break;
	case CTLTYPE_LONG:
	case CTLTYPE_ULONG:
		jp->jp_valuelen = sizeof(long);
		break;
	case CTLTYPE_QUAD:
		jp->jp_valuelen = sizeof(int64_t);
		break;
	case CTLTYPE_STRING:
		desc.s[0] = 0;
		desclen = sizeof(desc.s);
		if (sysctl(mib + 2, miblen / sizeof(int), desc.s, &desclen,
		    NULL, 0) < 0) {
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "sysctl(" SJPARAM ".%s): %s", jp->jp_name,
			    strerror(errno));
			return (-1);
		}
		jp->jp_valuelen = strtoul(desc.s, NULL, 10);
		break;
	case CTLTYPE_STRUCT:
		if (!strcmp(desc.s, "S,in_addr")) {
			jp->jp_structtype = JPS_IN_ADDR;
			jp->jp_valuelen = sizeof(struct in_addr);
		} else if (!strcmp(desc.s, "S,in6_addr")) {
			jp->jp_structtype = JPS_IN6_ADDR;
			jp->jp_valuelen = sizeof(struct in6_addr);
		} else {
			desclen = 0;
			if (sysctl(mib + 2, miblen / sizeof(int),
			    NULL, &jp->jp_valuelen, NULL, 0) < 0) {
				snprintf(jail_errmsg, JAIL_ERRMSGLEN,
				    "sysctl(" SJPARAM ".%s): %s", jp->jp_name,
				    strerror(errno));
				return (-1);
			}
		}
		break;
	case CTLTYPE_NODE:
		/* A node might be described by an empty-named child. */
		mib[1] = 1;
		mib[(miblen / sizeof(int)) + 2] =
		    mib[(miblen / sizeof(int)) + 1] - 1;
		miblen += sizeof(int);
		desclen = sizeof(desc.s);
		if (sysctl(mib, (miblen / sizeof(int)) + 2, desc.s, &desclen,
		    NULL, 0) < 0) {
			snprintf(jail_errmsg, JAIL_ERRMSGLEN,
			    "sysctl(0.1): %s", strerror(errno));
			return (-1);
		}
		if (desc.s[desclen - 2] != '.')
			goto unknown_parameter;
		goto mib_desc;
	default:
		snprintf(jail_errmsg, JAIL_ERRMSGLEN,
		    "unknown type for %s", jp->jp_name);
		errno = ENOENT;
		return (-1);
	}
	if (isarray) {
		jp->jp_elemlen = jp->jp_valuelen;
		jp->jp_valuelen = 0;
	}
	return (0);
}

/*
 * Change a boolean parameter name into its "no" counterpart or vice versa.
 */
static char *
noname(const char *name)
{
	char *nname, *p;

	nname = malloc(strlen(name) + 3);
	if (nname == NULL) {
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (NULL);
	}
	p = strrchr(name, '.');
	if (p != NULL)
		sprintf(nname, "%.*s.no%s", (int)(p - name), name, p + 1);
	else
		sprintf(nname, "no%s", name);
	return (nname);
}

static char *
nononame(const char *name)
{
	char *p, *nname;

	p = strrchr(name, '.');
	if (strncmp(p ? p + 1 : name, "no", 2)) {
		snprintf(jail_errmsg, sizeof(jail_errmsg),
		    "mismatched boolean: %s", name);
		errno = EINVAL;
		return (NULL);
	}
	nname = malloc(strlen(name) - 1);
	if (nname == NULL) {
		strerror_r(errno, jail_errmsg, JAIL_ERRMSGLEN);
		return (NULL);
	}
	if (p != NULL)
		sprintf(nname, "%.*s.%s", (int)(p - name), name, p + 3);
	else
		strcpy(nname, name + 2);
	return (nname);
}
