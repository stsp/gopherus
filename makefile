all:
	$(MAKE) -f Makefile.d64
	$(MAKE) -C dj32

clean:
	$(MAKE) -f Makefile.d64 clean
	$(MAKE) -C dj32 clean
