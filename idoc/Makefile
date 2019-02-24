
CFLAGS = -Wall -Wextra -pedantic -O2 -std=gnu89

all: idoc.h

idoc.h: idocgen
	./idocgen welcome.txt idoc_welcome >> idoc.h
	./idocgen ../gopherus.txt idoc_manual >> idoc.h

idocgen: idocgen.c

clean:
	rm -f idocgen idoc.h idict.h
