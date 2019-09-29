all:
	gcc -Wall main.c -lliquid -lc -lm -lcorrect
clean:
	rm ./a.out