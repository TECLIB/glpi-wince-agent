
MAKE = make

all:
	@$(MAKE) -w -C src strip
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll
	@$(MAKE) -w -C src glpi-wince-agent-service.dll
	#@$(MAKE) -w -C src glpi-wince-agent-cpl.dll

debug:
	@$(MAKE) -w -C src DEBUG=-DDEBUG

test:
	@$(MAKE) -w -C src strip TEST=1 
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll TEST=1
	@$(MAKE) -w -C src glpi-wince-agent-service.dll TEST=1
	#@$(MAKE) -w -C src glpi-wince-agent-cpl.dll TEST=1

tools:
	@$(MAKE) -w -C src debug-registry.exe

clean:
	@$(MAKE) -w -C src clean

cpl:
	@$(MAKE) -w -C src glpi-wince-agent-cpl.dll

setup:
	@$(MAKE) -w -C src glpi-wince-agent-setup.dll

service:
	@$(MAKE) -w -C src glpi-wince-agent-service.dll

cab: clean all
ifdef USE_UPX
	@tools/makecab-release.sh --upx
else
	@tools/makecab-release.sh
endif

cabtest: clean test
ifdef USE_UPX
	@tools/makecab-release.sh --upx --test
else
	@tools/makecab-release.sh --test
endif

release: cab cabtest

glpi-wince-agent.tar.gz: README.md Makefile src/*.{c,h,rc}

.PHONY: all debug clean release test cab cabtest cpl tools
