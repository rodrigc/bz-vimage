# $FreeBSD: src/tools/tools/nanobsd/gateworks/Files/root/.profile,v 1.2 2009/11/13 05:54:55 ed Exp $
#
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/games:/usr/local/sbin:/usr/local/bin:~/bin
export PATH
HOME=/root; export HOME
TERM=${TERM:-xterm}; export TERM
PAGER=more; export PAGER

#set -o vi
set -o emacs
if [ `id -u` = 0 ]; then
    PS1="`hostname -s`# "
else
    PS1="`hostname -s`% "
fi
