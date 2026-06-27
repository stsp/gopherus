all:
	$(MAKE) -f Makefile.d64
	$(MAKE) -C dj32

clean:
	$(MAKE) -f Makefile.d64 clean
	$(MAKE) -C dj32 clean

install:
	$(MAKE) -f Makefile.d64 install
	$(MAKE) -C dj32 install

uninstall:
	$(MAKE) -f Makefile.d64 uninstall
	$(MAKE) -C dj32 uninstall

deb:
	debuild -i -us -uc -b
