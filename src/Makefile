MAINS   := test.c snscript.c
SOURCES := $(filter-out $(MAINS),$(wildcard *.c))
HEADERS := $(wildcard *.h)

.PHONY: all
all: test snscript

test: test.c $(SOURCES) $(HEADERS)
	gcc -Wall -Werror -g test.c $(SOURCES) -o $@

snscript: snscript.c $(SOURCES) $(HEADERS)
	gcc -Wall -Werror -g snscript.c $(SOURCES) -o $@

.PHONY: clean
clean:
	rm -f test snscript
