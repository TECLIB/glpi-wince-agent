
MAKE = make

all:
	@$(MAKE) -w -C src strip STATIC=-static

debug:
	@$(MAKE) -w -C src DEBUG=-DDEBUG=2 STATIC=-static

test:
	@$(MAKE) -w -C src TEST=-DTEST strip STATIC=-static

clean:
	@$(MAKE) -w -C src clean

release:
	@$(MAKE) -w -C src clean
	@$(MAKE) debug
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent-debug.exe
	@$(MAKE) -w -C src clean
	@$(MAKE) all
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent.exe
	@$(MAKE) -w -C src clean
	@$(MAKE) test
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent-test.exe

glpi-wince-agent.tar.gz: README.md Makefile src/*.{c,h,rc}

.PHONY: all debug clean release test
