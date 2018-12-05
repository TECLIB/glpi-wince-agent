#! /bin/bash

let TEST=0 UPX=0 LCAB=0
while [ -n "$1" ]
do
	[ "$1" == "--test" ] && let TEST++
	[ "$1" == "--upx" ]  && let UPX++
	[ "$1" == "--lcab" ] && let LCAB++
	shift
done

set -e

# Change to project folder
cd "${0%/*}/.."

LCAB_URL="https://launchpad.net/ubuntu/+archive/primary/+files/lcab_1.0b12.orig.tar.gz"
CABWIZ_URL="https://github.com/nakomis/cabwiz/archive/master.zip"

# Check we have lcab built under tools folder
if [ $LCAB -gt 0 -a ! -x tools/lcab ]; then
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
if [ $LCAB -gt 0 -a ! -x tools/cabwiz ]; then
	[ -e cabwiz.zip ] || wget -O cabwiz.zip "$CABWIZ_URL"
	unzip -ju cabwiz.zip -x cabwiz-master/{.gitignore,LICENSE,README.md} -d tools
fi

export PATH=$PATH:tools

read define major_string MAJOR <<<$( egrep '^#define MAJOR_VERSION' src/glpi-wince-agent.h )
read define minor_string MINOR <<<$( egrep '^#define MINOR_VERSION' src/glpi-wince-agent.h )

if [ "$UPX" -ne 0 -a -x "$( which upx 2>/dev/null )" ]; then
	upx --compress-icons=0 --best src/glpi-wince-agent.exe
	[ -e src/glpi-wince-agent-cpl.dll ] && upx --compress-icons=0 --best src/glpi-wince-agent-cpl.dll
	upx --best src/glpi-wince-agent-setup.dll
	upx --best src/glpi-wince-agent-service.dll
fi

# Install built files under build folder with installation name
rm -rf build
mkdir build
cp -a src/glpi-wince-agent-service.dll build/glpi-agent.dll
cp -a src/glpi-wince-agent-setup.dll build/setup.dll
cp -a src/glpi-wince-agent.exe build/glpi-agent.exe
[ -e src/glpi-wince-agent-cpl.dll ] && cp -a src/glpi-wince-agent-cpl.dll build/glpi-agent.cpl

# Update inf file
sed -e "s/^AgentVersion = .*/AgentVersion = \"$MAJOR.$MINOR\"/" \
	src/glpi-wince-agent.inf >build/glpi-wince-agent.inf

# Generate cab
: ${WINE:=$( type -p wine )}
if [ $LCAB -eq 0 -a -e tools/cabwiz.exe -a -n "$WINE" ]; then
	if [ ! -e tools/cabwiz.ddf ]; then
		echo "You should also install cabwiz.ddf file in tools folder" >&2
	fi
	if [ ! -e tools/makecab.exe ]; then
		echo "You should also install makecab.exe file in tools folder" >&2
	fi

	cd build
	ln -s . build
	$WINE ../tools/cabwiz.exe glpi-wince-agent.inf
	cd ..
	if [ ! -e build/glpi-wince-agent.CAB ]; then
		echo "Failed to generate CAB file" >&2
		exit 1
	fi
	mv -vf build/glpi-wince-agent.CAB GLPI-Agent.cab
else
	# Try
	cabwiz build/glpi-wince-agent.inf /v
	if [ ! -e GLPI-Agent.cab ]; then
		echo "Failed to generate CAB file" >&2
		exit 1
	fi
fi

# Rename produced CAB file
if (( TEST )); then
	mv -vf GLPI-Agent.cab glpi-agent-test-v$MAJOR.$MINOR.cab
else
	mv -vf GLPI-Agent.cab glpi-agent-v$MAJOR.$MINOR.cab
fi
