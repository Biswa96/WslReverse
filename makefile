#makefile for WslReverse project

CC = gcc
CFLAGS=-Os -Wall -Werror -Wextra -Wundef
LIBS=-lOle32
BIN=bin
SRC=src
OBJS= \
	$(BIN)\functions.obj \
	$(BIN)\wgetopt.obj \
	$(BIN)\WslReverse.obj

WslReverse: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BIN)\$@ $(LIBS)

$(BIN)\functions.obj: $(SRC)\functions.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)\wgetopt.obj: $(SRC)\wgetopt.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)\WslReverse.obj: $(SRC)\WslReverse.c
	$(CC) $(CFLAGS) -c $< -o $@
