# $FreeBSD$

PREFIX?=	/usr/local
MK_DEBUG_FILES=	no

PROG=		bh1750d
BINDIR=		${PREFIX}/sbin

MAN=		${PROG}.8
MANDIR=		${PREFIX}/man/man

CFLAGS+=	-Wall -I/usr/local/include
LDADD=		-L/usr/local/lib -lutil -lsqlite3

uninstall:
	rm ${BINDIR}/${PROG}
	rm ${MANDIR}8/${MAN}.gz

check:
	cppcheck \
	    --enable=all \
	    --force \
	    -USQLITE_INT64_TYPE \
	    -USQLITE_UINT64_TYPE \
	    -I/usr/local/include \
	    ./

.include <bsd.prog.mk>
