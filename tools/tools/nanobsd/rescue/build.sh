#!/bin/sh
#
# $FreeBSD: src/tools/tools/nanobsd/rescue/build.sh,v 1.1 2010/10/01 10:34:35 mr Exp $
#

if [ -z "${1}" -o \! -f "${1}" ]; then
  echo "Usage: $0 cfg_file [-bhiknw]"
  echo "-i : skip image build"
  echo "-w : skip buildworld step"
  echo "-k : skip buildkernel step"
  echo "-b : skip buildworld and buildkernel step"
  exit
fi

CFG="${1}"
shift;

sh ../nanobsd.sh $* -c ${CFG}
