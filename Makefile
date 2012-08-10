CFLAGS:=-std=c99 -Wall -Wextra -pedantic -O2 ${CFLAGS}
LDLIBS:=-lalpm
PREFIX?=/usr/local

OUT=pacrat
SRC=${wildcard *.c}
OBJ=${SRC:.c=.o}

${OUT}: ${OBJ}

install: ${OUT}
	install -D -m755 ${OUT} ${DESTDIR}${PREFIX}/bin/${OUT}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${OUT}

clean:
	${RM} ${OUT} ${OBJ}

.PHONY: clean install uninstall
