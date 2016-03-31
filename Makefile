
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
	@$(MAKE) clean setup all
	@tools/makecab-release.sh

cabtest:
	@$(MAKE) clean setup test
	@tools/makecab-release.sh --test

release:
	@$(MAKE) clean debug
	@mv -vf src/glpi-wince-agent.exe glpi-wince-agent-debug.exe
	@$(MAKE) clean setup all
	@cp -avf src/glpi-wince-agent.exe glpi-wince-agent.exe
	@tools/makecab-release.sh
	@$(MAKE) clean setup test
	@cp -avf src/glpi-wince-agent.exe glpi-wince-agent-test.exe
	@tools/makecab-release.sh --test

glpi-wince-agent.tar.gz: README.md Makefile src/*.{c,h,rc}

.PHONY: all debug clean release test cab cabtest
