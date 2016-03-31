
MAKE = make

all:
	@$(MAKE) -w -C src strip STATIC=-static

debug:
	@$(MAKE) -w -C src DEBUG=-DDEBUG=2 STATIC=-static

test:
	@$(MAKE) -w -C src TEST=-DTEST strip STATIC=-static

clean:
	@$(MAKE) -w -C src clean

setup:
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll STATIC=-static

cab:
	@$(MAKE) clean
	@$(MAKE) setup
	@$(MAKE) all
	@tools/makecab-release.sh

release:
	@$(MAKE) clean
	@$(MAKE) debug
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent-debug.exe
	@$(MAKE) clean
	@$(MAKE) all
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent.exe
	@$(MAKE) clean
	@$(MAKE) test
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent-test.exe
	@$(MAKE) cab

glpi-wince-agent.tar.gz: README.md Makefile src/*.{c,h,rc}

.PHONY: all debug clean release test cab
