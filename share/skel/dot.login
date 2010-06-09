# $FreeBSD: src/share/skel/dot.login,v 1.17 2009/03/27 21:13:14 ru Exp $
#
# .login - csh login script, read by login shell, after `.cshrc' at login.
#
# see also csh(1), environ(7).
#

if ( -x /usr/games/fortune ) /usr/games/fortune freebsd-tips
