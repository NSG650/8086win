CC := gcc
CFLAGS := -Iinclude

CFILES := $(shell find -path -prune -type f -o -name '*.c')
OBJ := $(CFILES:.c=.o)
HEADER_DEPS :=  $(CFILES:.c=.d)

.PHONY: all
all: 8086win

8086win: $(OBJ)
	$(CC) $(OBJ) -o $@

-include $(HEADER_DEPS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf 8086win $(OBJ) $(HEADER_DEPS)

.PHONY: run
run: all
	./8086win
