# Makefile for WslReverse project

NAME := WslReverse
BINDIR = ../bin
CC = gcc
CFLAGS := -Os -std=c99 -m64 -Wall -Wextra
LIBS := -lntdll -lole32 -lshell32 -lws2_32
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.obj)
BINS := $(patsubst %.obj,$(BINDIR)/%.obj,$(OBJECTS))

# Msys2 specific options
CCOPT:= -D_WIN32_WINNT=0x0A000006 -DNTDDI_VERSION=0x0A000006

EXECUTABLE=$(BINDIR)/$(NAME)

all: $(BINDIR) $(EXECUTABLE)

$(EXECUTABLE): $(BINS)
	$(CC) $(CFLAGS) $(CCOPT) $^ $(LIBS) -o $@

$(BINS): $(BINDIR)/%.obj: %.c
	$(CC) -c $(CFLAGS) $(CCOPT) -fstack-check $< -o $(BINDIR)/$*.obj

$(BINDIR):
	@mkdir ..\bin

clean:
	rm -rf $(BINDIR)

#END
