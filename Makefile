sources := $(wildcard *.c)
objects := $(sources:%.c=%.o)
program := program

CC ?= gcc
O ?= g
CFLAGS := -O$(O) -ffunction-sections -fdata-sections -Wall -Wextra
LDFLAGS := -O$(O) -Wl,--gc-sections -Wall -Wextra


ifeq ($(O),g)

CFLAGS += -g
LDFLAGS += -g

CFLAGS += -fsanitize=undefined
LDFLAGS += -lubsan

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
ENV+=LD_PRELOAD=/usr/lib/libasan.so

else

CFLAGS += -s
LDFLAGS += -s

endif


.PHONY: build clean run

build: $(program)

run: $(program)
	valgrind -q env $(ENV) ./$(program)

$(program): $(objects)
	$(CC) $(LDFLAGS) -MMD -o $@ $^
	size $@

$(objects): %.o: %.c
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

-include: $(wildcard *.d)

clean:
	rm -f -- *.o *.d $(program)
