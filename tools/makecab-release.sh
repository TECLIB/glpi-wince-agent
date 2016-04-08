#! /bin/bash

let TEST=0
[ "$1" == "--test" ] && let TEST++

set -e

# Change to project folder
cd "${0%/*}/.."

LCAB_URL="https://launchpad.net/ubuntu/+archive/primary/+files/lcab_1.0b12.orig.tar.gz"
CABWIZ_URL="https://github.com/Turbo87/cabwiz/archive/master.zip"

# Check we have lcab built under tools folder
if [ ! -x tools/lcab ]; then
	SRCS="${LCAB_URL##*/}"
	if [ ! -d lcab ]; then
		[ -e "$SRCS" ] || wget -nd "$LCAB_URL"
		mkdir lcab
		tar xzf "$SRCS" --strip-components=1 -C lcab
	fi
	cd lcab
	./configure
	make
	mv lcab ../tools
	cd ..
	rm -rf lcab
fi

# Check we have cabwiz replacement tool under tools folder
if [ ! -x tools/cabwiz ]; then
	[ -e cabwiz.zip ] || wget -O cabwiz.zip "$CABWIZ_URL"
	unzip -ju cabwiz.zip -x cabwiz-master/{.gitignore,LICENSE,README.md} -d tools
fi

export PATH=$PATH:./tools

read define major_string MAJOR <<<$( egrep '^#define MAJOR_VERSION' src/glpi-wince-agent.h )
read define minor_string MINOR <<<$( egrep '^#define MINOR_VERSION' src/glpi-wince-agent.h )

# Update inf file
sed -e "s/^AgentVersion = .*/AgentVersion = \"$MAJOR.$MINOR\"/" \
	src/glpi-wince-agent.inf >glpi-wince-agent.inf

if [ -x "$( which upx 2>/dev/null )" ]; then
	upx --compress-icons=0 --best src/glpi-wince-agent.exe
	upx  --best src/glpi-wince-agent-setup.dll
	upx --best src/glpi-wince-agent-service.dll
fi

# Generate cab
tools/cabwiz glpi-wince-agent.inf /v

# Rename produced CAB file
if (( TEST )); then
	mv -vf GLPI-Agent.CAB glpi-agent-test-v$MAJOR.$MINOR.cab
else
	mv -vf GLPI-Agent.CAB glpi-agent-v$MAJOR.$MINOR.cab
fi

# Cleanup
rm -f glpi-wince-agent.inf
