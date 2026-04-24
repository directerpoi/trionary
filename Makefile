CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -Iinclude
SRCS    = src/main.c src/reader.c src/lexer.c src/parser.c src/exec.c src/output.c
TARGET  = tri

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -lm

clean:
	rm -f $(TARGET)