SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)

test: $(SOURCES) $(HEADERS)
	gcc -Wall -Werror -g $(SOURCES) -o $@

.PHONY: clean
clean:
	rm -f test
