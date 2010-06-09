# $FreeBSD: src/tools/regression/lib/msun/Makefile,v 1.15 2009/01/31 18:31:57 das Exp $

TESTS=	test-conj test-csqrt test-exponential test-fenv test-fma \
	test-fmaxmin test-ilogb test-invtrig test-lrint \
	test-lround test-nan test-next test-rem test-trig
CFLAGS+= -O0 -lm

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}
