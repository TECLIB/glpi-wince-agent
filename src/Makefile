
.SUFFIXES: .exe .c .h .rc .rsc .C

ARCH    = arm-mingw32ce
CC      = ${ARCH}-gcc
WINDRES = ${ARCH}-windres
STRIP   = ${ARCH}-strip
CFLAGS  = -g ${STATIC} ${DEBUG} -Wall -DWIN32 -D_WIN32_WINNT=0x0501
LIBS    = -lcommctrl
STATIC  = 
DEBUG   = 

OBJECTS = glpi-wince-agent.o agent.o config.o constants.o logger.o tools.o

all: glpi-wince-agent.exe

strip: glpi-wince-agent.exe
	${STRIP} $?

glpi-wince-agent.exe: ${OBJECTS} glpi-wince-agent.rsc
	${CC} ${CFLAGS} -o $@ ${OBJECTS} glpi-wince-agent.rsc ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c $?

glpi-wince-agent.rsc: glpi-wince-agent.rc glpi-wince-agent.h
	${WINDRES} glpi-wince-agent.rc glpi-wince-agent.rsc

clean:
	@rm -vf *.o *.exe *.rsc

.PHONY: all strip clean
