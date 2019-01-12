# Makefile for WslReverse project

NAME := WslReverse
BINDIR = ../bin
CC = gcc
CFLAGS := -ggdb -m64 -Wall -Wextra
LIBS := -lOle32 -lNtdll
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

#END
