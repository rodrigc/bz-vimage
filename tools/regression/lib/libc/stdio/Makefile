# $FreeBSD: src/tools/regression/lib/libc/stdio/Makefile,v 1.7 2009/02/28 06:39:39 das Exp $

TESTS=	test-getdelim test-perror test-print-positional test-printbasic test-printfloat test-scanfloat
CFLAGS+= -lm

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}
