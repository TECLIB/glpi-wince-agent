
.SUFFIXES: .exe .c .h .rc .rsc .C

ARCH    = arm-mingw32ce
CC      = ${ARCH}-gcc
WINDRES = ${ARCH}-windres
STRIP   = ${ARCH}-strip
CFLAGS  = -g ${STATIC} ${DEBUG} -Wall -D_WIN32_WINNT=0x0501
LIBS    = -lcommctrl
STATIC  = 
DEBUG   = 

all: glpi-wince-agent.exe

strip: glpi-wince-agent.exe
	${STRIP} $?

glpi-wince-agent.exe: glpi-wince-agent.o glpi-wince-agent.rsc
	${CC} ${CFLAGS} -o $@ glpi-wince-agent.o glpi-wince-agent.rsc ${LIBS}

glpi-wince-agent.o: glpi-wince-agent.c
	${CC} ${CFLAGS} -c $?

glpi-wince-agent.rsc: glpi-wince-agent.rc glpi-wince-agent.h
	${WINDRES} glpi-wince-agent.rc glpi-wince-agent.rsc

clean:
	@rm -vf *.o *.exe *.rsc

.PHONY: all strip clean
