
MAKE = make

all:
	@$(MAKE) -w -C src strip STATIC=-static

debug:
	@$(MAKE) -w -C src DEBUG=-DDEBUG=2

clean:
	@$(MAKE) -w -C src clean

glpi-wince-agent.tar.gz: README.md Makefile src/*.{c,h,rc}

.PHONY: all debug clean
