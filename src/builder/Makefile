# Builder Makefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I../include
LDFLAGS = -L../lib
LDLIBS = -lssl -lcrypto

SRCS = builder.c builder_main.c template_generator.c client_template.c
OBJS = $(SRCS:.c=.o)
TARGET = builder

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
