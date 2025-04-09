
all: zip_checker

zip_checker: main.c
	$(CC) $^ -o $@ -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f zip_checker core

.PHONY: all clean
