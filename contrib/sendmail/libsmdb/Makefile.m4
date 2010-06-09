dnl $Id$
include(confBUILDTOOLSDIR`/M4/switch.m4')

define(`confREQUIRE_SM_OS_H', `true')
# sendmail dir
SMSRCDIR=	ifdef(`confSMSRCDIR', `confSMSRCDIR', `${SRCDIR}/sendmail')
PREPENDDEF(`confENVDEF', `confMAPDEF')
PREPENDDEF(`confINCDIRS', `-I${SMSRCDIR} ')

bldPRODUCT_START(`library', `libsmdb')
define(`bldSOURCES', `smdb.c smdb1.c smdb2.c smndbm.c ')
APPENDDEF(`confENVDEF', `-DNOT_SENDMAIL')
bldPRODUCT_END

bldFINISH
