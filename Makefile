# $FreeBSD$

PREFIX?= /usr/local
MK_DEBUG_FILES=	no

PROG=	bh1750-daemon
BINDIR=	${PREFIX}/sbin

SCRIPTS= bh1750-control.py ${PROG}.sh

SCRIPTSNAME_${PROG}.sh=		${PROG}
SCRIPTSDIR_${PROG}.sh=		${PREFIX}/etc/rc.d
SCRIPTSNAME_bh1750-control.py=	bh1750-control
SCRIPTSDIR_bh1750-control.py=	${PREFIX}/bin

MAN=	${PROG}.8
MANDIR=	${PREFIX}/man/man

CFLAGS+= -Wall -I/usr/local/include
LDADD=	-L/usr/local/lib -lc -lutil -lsqlite3

uninstall:
	rm ${BINDIR}/${PROG}
	rm ${MANDIR}8/${MAN}.gz
	rm ${PREFIX}/etc/rc.d/${PROG}
	rm ${SCRIPTSDIR_bh1750-control.py}/${SCRIPTSNAME_bh1750-control}

check:
	cppcheck \
	    --enable=all \
	    --force \
	    -USQLITE_INT64_TYPE \
	    -USQLITE_UINT64_TYPE \
	    -I/usr/local/include \
	    ./

.include <bsd.prog.mk>
