# Makefile for common files

NAME = common
BINDIR = ../bin/$(NAME)
CFLAGS = -Os -std=c99 -m64 -Wall -Wextra
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.obj)
BINS = $(patsubst %.obj,$(BINDIR)/%.obj,$(OBJECTS))

# Msys2 specific options
CCOPT = -D_WIN32_WINNT=0x0A000007 -DNTDDI_VERSION=0x0A000007

all: $(BINDIR) $(BINS)

$(BINS): $(BINDIR)/%.obj: %.c
	$(CC) -c $(CFLAGS) $(CCOPT) $< -o $(BINDIR)/$*.obj

$(BINDIR):
	@mkdir -p $(BINDIR)

# END