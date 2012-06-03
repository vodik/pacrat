# pacrat - pacman config manager

OUT=pacrat
VERSION=$(shell git describe)

SRC=${wildcard *.c}
OBJ=${SRC:.c=.o}
DISTFILES=Makefile pacrat.c

PREFIX?=/usr/local
MANPREFIX?=${PREFIX}/share/man

CPPFLAGS:=-DPACRAT_VERSION=\"${VERSION}\" ${CPPFLAGS}
CFLAGS:=-std=gnu99 -g -pedantic -Wall -Wextra ${CFLAGS} -g
LDFLAGS:=-lcurl -lalpm ${LDFLAGS}

all: ${OUT} doc

${OUT}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

doc: pacrat.1
pacrat.1: README.pod
	pod2man --section=1 --center="Pacrat Manual" --name="PACRAT" --release="pacrat ${VERSION}" $< > $@

strip: ${OUT}
	strip --strip-all ${OUT}

install: pacrat pacrat.1
	install -D -m755 pacrat ${DESTDIR}${PREFIX}/bin/pacrat
	install -D -m644 pacrat.1 ${DESTDIR}${MANPREFIX}/man1/pacrat.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	rm -f ${DESTDIR}${PREFIX}/bin/pacrat
	@echo removing man page from ${DESTDIR}${MANPREFIX}/man1/pacrat.1
	rm -f ${DESTDIR}/${MANPREFIX}/man1/pacrat.1

dist: clean
	mkdir pacrat-${VERSION}
	cp ${DISTFILES} pacrat-${VERSION}
	sed "s/\(^VERSION *\)= .*/\1= ${VERSION}/" Makefile > pacrat-${VERSION}/Makefile
	tar czf pacrat-${VERSION}.tar.gz pacrat-${VERSION}
	rm -rf pacrat-${VERSION}

distcheck: dist
	tar xf pacrat-${VERSION}.tar.gz
	${MAKE} -C pacrat-${VERSION}
	rm -rf pacrat-${VERSION}

clean:
	${RM} ${OUT} ${OBJ} pacrat.1

.PHONY: clean dist doc install uninstall
