# xarm — builds natively inside SimpleOSv5 (Alpine i386, musl, gcc).
# `make CC=cc` on the host also works as a quick compile check.

PROG     := xarm
BUILD    := build

CC       ?= cc
CFLAGS   ?= -std=gnu11 -O2 -g -Wall -Wextra
CPPFLAGS += -MMD -MP
LDFLAGS  ?=
LDLIBS   ?=

SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean
all: $(BUILD)/$(PROG)
	chmod +x $<

$(BUILD)/$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

-include $(DEPS)
