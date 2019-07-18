all:
	gcc -Wall main.c -lliquid -lc -lm
clean:
	rm ./a.out