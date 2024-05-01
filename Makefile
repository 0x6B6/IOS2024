CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt

SRC = main.c utility.c
OBJ = main.o utility.o
EXECUTABLE = proj2

LOGIN = proj2
OUTPUT_FILE = proj2.out

all: $(EXECUTABLE)
	@echo "Project compiled successfuly!"

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%o : %c
	$(CC) $(CFLAGS) -c $^

pack:
	zip $(LOGIN).zip *.c *.h Makefile

clean:
	rm -f *.o $(EXECUTABLE) $(OUTPUT_FILE) $(LOGIN).zip

.PHONY: all pack clean
