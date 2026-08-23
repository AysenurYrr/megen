CC = gcc

CPPFLAGS = -Isrc -Itoml_parser
CFLAGS = -Wall -Wextra -std=c11

TARGET = megen

SRC = \
	src/main.c \
	src/profile.c \
	src/profile_dump.c \
	src/latex.c \
	src/html.c

all: $(TARGET)

$(TARGET): $(SRC) src/profile.h src/latex.h src/html.h web/style.css web/gallery.js
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
