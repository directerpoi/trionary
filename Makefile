CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
SRCS    = main.c reader.c lexer.c parser.c exec.c output.c
TARGET  = tri

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -lm

clean:
	rm -f $(TARGET)