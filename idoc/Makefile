
CFLAGS = -Wall -Wextra -pedantic -O2

all: idoc.h

idoc.h: idocgen
	./gendict.sh
	./idocgen license.txt idoc_license > idoc.h
	./idocgen welcome.txt idoc_welcome >> idoc.h
	./idocgen ../gopherus.txt idoc_manual >> idoc.h

idocgen: idocgen.c

clean:
	rm -f idocgen idoc.h
