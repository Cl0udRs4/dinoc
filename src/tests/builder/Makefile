# Builder Tests Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I../../include
LDFLAGS = 
LDLIBS = -lssl -lcrypto -lrt -lpthread -luuid

SRCS = test_builder.c ../../builder/builder.c ../../builder/template_generator.c ../../builder/client_template.c ../../builder/signature.c
OBJS = $(SRCS:.c=.o)
TARGET = test_builder

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
