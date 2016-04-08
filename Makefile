
MAKE = make

all:
	@$(MAKE) -w -C src strip
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll
	@$(MAKE) -w -C src glpi-wince-agent-service.dll STDERR=$(STDERR)

debug:
	@$(MAKE) -w -C src DEBUG=-DDEBUG

test:
	@$(MAKE) -w -C src TEST=-DTEST strip
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll TEST=-DTEST
	#@$(MAKE) -w -C src glpi-wince-agent-service.dll TEST=-DTEST
	@$(MAKE) -w -C src glpi-wince-agent-service.dll TEST="-DTEST -DSTDERR"

clean:
	@$(MAKE) -w -C src clean

setup:
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll

service:
	@$(MAKE) -w -C src glpi-wince-agent-service.dll

cab:
	@$(MAKE) clean all
	@tools/makecab-release.sh

cabtest:
	@$(MAKE) clean test
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
