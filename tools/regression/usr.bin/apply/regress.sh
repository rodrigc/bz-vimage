# $FreeBSD: src/tools/regression/usr.bin/apply/regress.sh,v 1.1 2010/03/05 15:23:01 jh Exp $

echo 1..2

REGRESSION_START($1)

REGRESSION_TEST(`00', `apply "echo %1 %1 %1 %1" $(cat regress.00.in)')
REGRESSION_TEST(`01', `sh regress.01.sh')

REGRESSION_END()
