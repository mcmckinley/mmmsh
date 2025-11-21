CC      := cc
TARGET  := mmmsh
SRC     := mmmsh.c

CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

DEBUG_FLAGS := -g3 -Og -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all debug clean # clarifying that these are commands not files

all: $(TARGET) # default target

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
# @ - target name of current rule
# ^ - prereqs of current rule	

# "make debug" rebuilds with sanitizer + debug info
debug: CFLAGS := $(filter-out -O%,$(CFLAGS)) $(DEBUG_FLAGS)
debug: clean $(TARGET)

clean:
	rm -f $(TARGET)
