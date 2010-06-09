# $FreeBSD: src/tools/regression/lib/libutil/Makefile,v 1.4 2009/11/12 01:37:25 des Exp $

TESTS=	test-trimdomain test-trimdomain-nodomain test-flopen test-grp test-pidfile
CFLAGS+= -g -Wall -Wextra -Werror -lutil

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}
