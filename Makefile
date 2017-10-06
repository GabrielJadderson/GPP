# Vælg installations folder, eller bare samme folder som kildekode
INSTALLDIR = $(shell pwd)
# Vælg størrelse af font i grafiske vinduer
FONT = "6x13"
# Vælg størrelse af grafiske vinduer
GEOM = "85x55"

LIBSUBNET = libsubnet.a
NETWORK = network
RM = rm -f
AR = ar rcs
MKDIR = mkdir -p
CP = cp
MV = mv
UNAME = $(shell uname)

SRC = Activate.c ClearEvent.c FromSubnet.c Signal.c Start.c Stop.c ToSubnet.c Wait.c \
	control.c delay_frame.c errortimer.c fifoqueue.c flow.c priqueue.c re_delay_frame.c \
	receiver.c shell.c signalhandler.c subnet.c subnetsupport.c timer.c transfer_frame.c \
	transmit_error.c

HEADERS = fifoqueue.h priqueue.h subnet.h subnetsupport.h

TESTS = nettest.c timertest.c signaltest.c maketest.c funk1.c

OBJS = $(SRC:.c=.o)
TOBJS = $(TESTS:.c=.o)
TOUT = $(TESTS:.c=)

# Compiler
ifeq "$(UNAME)" "SunOS"
CC = cc
else
CC = gcc
endif

DEFS = -D_GNU_SOURCE -DGEOM='$(GEOM)' -DFONT='$(FONT)'

# Linker
ifeq "$(UNAME)" "SunOS"
CFLAGS  = -O3 $(DEFS) -I. -I./include
LDFLAGS += -L. -L./lib -lsubnet -lxnet
endif
ifeq "$(UNAME)" "Darwin"
CFLAGS  = -O3 -Wall -pedantic $(DEFS) -I. -I./include
LDFLAGS += -L. -L./lib -lsubnet -pthread
else
CFLAGS  = -O3 -Wall -pedantic $(DEFS) -I. -I./include
LDFLAGS += -L. -L./lib -pthread -lsubnet
endif

.PHONY:	all tests install uninstall full-uninstall clean

all:	$(LIBSUBNET) \
	$(NETWORK)

tests:	$(INSTALLDIR)/lib/$(LIBSUBNET) \
	nettest \
	timertest \
	signaltest \
	maketest

install:	$(LIBSUBNET)
	@echo "Installing in $(INSTALLDIR)"
	@$(MKDIR) $(INSTALLDIR)/lib $(INSTALLDIR)/include
	@$(MV) $(LIBSUBNET) $(INSTALLDIR)/lib
	@for file in $(HEADERS); \
	do \
		$(CP) "$$file" $(INSTALLDIR)/include; \
	done

uninstall:
	@echo "Uninstalling from $(INSTALLDIR)"
	@$(RM) $(INSTALLDIR)/lib/$(LIBSUBNET)
	@for file in $(HEADERS); \
	do \
		$(RM) $(INSTALLDIR)/include/"$$file"; \
	done

full-uninstall: uninstall
	@$(RM) -r $(INSTALLDIR)/include $(INSTALLDIR)/lib

$(NETWORK):	network.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

nettest:	nettest.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

timertest:	timertest.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

signaltest:	signaltest.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

maketest:	maketest.o funk1.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

mynetwork:	rdt.o debug.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


clean:
	@echo "Cleaning directory"
	@$(RM) $(OBJS) $(TOBJS) $(TOUT) $(LIBSUBNET) $(NETWORK) $(NETWORK).o \
	*.log.* *.log *~ core *Socket mynetwork

$(LIBSUBNET):	$(OBJS)
	$(AR) $@ $?

$(INSTALLDIR)/lib/$(LIBSUBNET):	install
