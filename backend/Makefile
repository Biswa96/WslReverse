# Makefile for backend executable

NAME = WslReverseHost
BINDIR = ../bin
CFLAGS = -Os -std=c99 -m64 -Wall -Wextra -I"../common"
LIBS = -lntdll -lole32 -lshell32 -lws2_32
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.obj)
BINS = $(patsubst %.obj,$(BINDIR)/%.obj,$(OBJECTS))
COMMON = $(wildcard $(BINDIR)/common/*.obj)

$(NAME): $(BINS) $(COMMON)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $(BINDIR)/$@

$(BINS): $(BINDIR)/%.obj: %.c
	$(CC) -c $(CFLAGS) $< -o $(BINDIR)/$*.obj

#END