CC = gcc
CFLAGS = -Wall -Iinclude -g
LDFLAGS = -lpthread

SRCS = src/main.c src/collector.c src/logger.c
OBJS = $(SRCS:src/%.c=build/%.o)
TARGET = build/log_tool

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build sensor_*.txt log_*.txt