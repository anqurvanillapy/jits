.PHONY: clean

calc:
	gcc -Wall -Wextra -pedantic calc.c -o calc

clean:
	rm -rf calc
