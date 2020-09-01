# $FreeBSD$

PREFIX?= /usr/local
MK_DEBUG_FILES= no

PROG= bh1750d
BINDIR= ${PREFIX}/sbin

MAN= ${PROG}.8
MANDIR= ${PREFIX}/man/man

uninstall:
	rm ${BINDIR}/${PROG}
	rm ${MANDIR}8/${MAN}.gz

.include <bsd.prog.mk>
