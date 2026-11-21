# Makefile for Chinese Text Editor (Linux / POSIX / macOS)
#
# Usage:
#   make        Build zero-dependency terminal editor (./editor)
#   make test   Build and run unit test suite
#   make clean  Remove build artifacts

CC      ?= cc
TARGET  := editor
BUILD   := build
UNAME_S := $(shell uname -s)

CFLAGS  := -std=c89 -O2 -Wall -Wextra -Wno-deprecated-declarations -I. -Iplatform

ifeq ($(UNAME_S),Darwin)
	LDLIBS := -liconv
else
	LDLIBS :=
endif

CORE_SRCS := \
	editer/drawmenu.c \
	editer/funckey.c \
	editer/hanzi.c \
	editer/inname.c \
	editer/language.c \
	editer/malloc.c \
	editer/mouse.c \
	editer/node.c \
	editer/open.c \
	editer/save.c \
	editer/spell.c \
	editer/submenus.c \
	editer/view.c

SRCS := $(CORE_SRCS) editer/editer2.c platform/bgi_tui.c
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test:
	@mkdir -p $(BUILD)/test
	$(CC) $(CFLAGS) -g tests/test_editor.c $(CORE_SRCS) -o $(BUILD)/test/test_editor
	./$(BUILD)/test/test_editor

clean:
	rm -rf $(BUILD) $(TARGET)
