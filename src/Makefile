
.SUFFIXES: .exe .c .h .rc .rsc .C

ARCH    = arm-mingw32ce
CC      = ${ARCH}-gcc
WINDRES = ${ARCH}-windres
STRIP   = ${ARCH}-strip
OBJDUMP = ${ARCH}-objdump
CFLAGS  = -g0 -Os -mpoke-function-name -mwin32 -Wall
CFLAGS += -DWIN32 -D_WIN32_WINNT=0x0501 -D_WIN32_WINCE=0x0501
LIBS    = -lcommctrl -liphlpapi -lws2 -lwininet
STATIC  = -static -static-libgcc
DEBUG   = 
TEST    = 
STDERR  = 
SETUP   = 

# CAB Installer server url preload configuration
# URL will be inserted in config file during first run if no config file is found
SERVER_URL = 
#SERVER_URL = http://glpi/plugins/fusioninventory/

# CAB Installer TerminalID search, set it to 1 to activate
SEARCH_TERMINALID = 

-include Makefile.local

ifneq ($(STATIC),)
	CFLAGS +=  ${STATIC}
endif

ifneq ($(STDERR),)
	CFLAGS += -DSTDERR
endif

ifneq ($(DEBUG),)
	CFLAGS += -DDEBUG
endif

ifneq ($(TEST),)
	CFLAGS += -DTEST
endif

ifneq ($(SERVER_URL),)
	SETUP += -DSERVER_URL='"${SERVER_URL}"'
endif

ifneq ($(SEARCH_TERMINALID),)
	SETUP += -DSEARCH_TERMINALID=${SEARCH_TERMINALID}
endif

CPLOBJS = glpi-wince-agent-cpl.o \
	glpi-wince-agent-cpl.rsc
EXEOBJS = glpi-wince-agent.o \
	agent.o config.o constants.o logger.o tools.o storage.o \
	glpi-wince-agent.rsc
SRVOBJS = glpi-wince-agent-service.lo \
	agent.lo config.lo constants.lo logger.lo tools.lo storage.lo target.lo \
	inventory.lo inventory/network.lo inventory/board.lo

all: glpi-wince-agent.exe

strip: glpi-wince-agent.exe
	${STRIP} $?

glpi-wince-agent.exe: ${EXEOBJS}
	${CC} ${CFLAGS} -o $@ $? ${LIBS}

debug-registry.exe: debug-registry.c
	${CC} ${CFLAGS} -o $@ $?
	${STRIP} $@

glpi-wince-agent-setup.dll: glpi-wince-agent-setup.c
	${CC} -shared ${CFLAGS} ${SETUP} -o $@ $?
	${STRIP} $@

glpi-wince-agent-cpl.dll: ${CPLOBJS}
	${CC} -shared ${CFLAGS} -o $@ $?
	${STRIP} $@

glpi-wince-agent-service.dll: ${SRVOBJS}
	${CC} -shared ${CFLAGS} -DGWA -o $@ $? ${LIBS}
	#${OBJDUMP} -w -d -S $@ >glpi-wince-agent-service.dis
	${STRIP} $@

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $?

%.lo: %.c
	${CC} -shared ${CFLAGS} -DGWA -c -o $@ $?

glpi-wince-agent.rsc: glpi-wince-agent.rc glpi-wince-agent.h
	${WINDRES} ${DEBUG} glpi-wince-agent.rc glpi-wince-agent.rsc

glpi-wince-agent-cpl.rsc: glpi-wince-agent-cpl.rc glpi-wince-agent.h
	${WINDRES} ${DEBUG} glpi-wince-agent-cpl.rc glpi-wince-agent-cpl.rsc

clean:
	@rm -vf *.o *.lo *.exe *.dll *.rsc inventory/*.o inventory/*.lo *.dis

.PHONY: all strip clean
