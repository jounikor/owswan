# Pardon my lame makefile.. it has been a while ;)
# 
# To compile
#    make
#
# To run in emu
#    make run

.PHONY: clean all run
.SUFFIXES:
.SUFFIXES: .c .h
.DEFAULT:
	make $(PROG)
#
SRCS := stripes.c
HDRS := 
OBJS := 

#
LOCAL_LIBS := libwscl.lib

#
PROG := stripes
BIN  := $(PROG).bin
ROM  := $(PROG).wsc
MAP  := $(PROG).map

#
CC := wcl
LD := wscromtool
RM := rm -f
CP := cp
FOO = Example $(ROM)

# Select between $2000:0000 and $4000:0000 compile targets
#TARGET := wscl		# $4000:0000
TARGET := wsc		# $2000:0000

LOCAL_CFLAGS = -fe=$(BIN) -fm=$(MAP) -bcl=$(TARGET) -1 -zc -ml -s -i=$(WATCOM)/h -i=$(WATCOM)/libwsc/h
LOCAL_LDFLAGS = rom -v

#
# rules

all: $(ROM)
	@echo $(FOO)

$(PROG): $(BIN)

$(ROM): $(BIN) $(MAP)
	$(LD) $(LOCAL_LDFLAGS) $(BIN) $(MAP) $(ROM)

$(BIN): $(SRCS)
	$(CC) $(LOCAL_CFLAGS) $< $(LOCAL_LIBS)


run: $(ROM)
	@mednafen $(ROM)

clean:
	-$(RM) $(BIN) $(MAP) $(ROM)
	-$(RM) *.o

dist:
	@echo distillery
