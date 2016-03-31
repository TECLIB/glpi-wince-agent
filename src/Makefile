
.SUFFIXES: .exe .c .h .rc .rsc .C

ARCH    = arm-mingw32ce
CC      = ${ARCH}-gcc
WINDRES = ${ARCH}-windres
STRIP   = ${ARCH}-strip
CFLAGS  = -g ${STATIC} ${DEBUG}${TEST} -Wall -DWIN32 -D_WIN32_WINNT=0x0501
LIBS    = -lcommctrl -liphlpapi -lws2 -lwininet
STATIC  = 
DEBUG   = 
TEST    = 

OBJECTS = glpi-wince-agent.o agent.o config.o constants.o logger.o tools.o \
	storage.o inventory.o inventory/network.o inventory/board.o target.o

all: glpi-wince-agent.exe

strip: glpi-wince-agent.exe
	${STRIP} $?

glpi-wince-agent.exe: ${OBJECTS} glpi-wince-agent.rsc
	${CC} ${CFLAGS} -o $@ ${OBJECTS} glpi-wince-agent.rsc ${LIBS}

glpi-wince-agent-setup.dll: glpi-wince-agent-setup.c
	${CC} -shared ${CFLAGS} -o $@ $?

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $?

glpi-wince-agent.rsc: glpi-wince-agent.rc glpi-wince-agent.h
	${WINDRES} ${DEBUG} glpi-wince-agent.rc glpi-wince-agent.rsc

clean:
	@rm -vf *.o *.exe *.dll *.rsc inventory/*.o

.PHONY: all strip clean
