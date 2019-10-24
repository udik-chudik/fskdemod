all:
	gcc -Wall src/main.c src/arguments.c src/helpers.c src/crc.c -Iinc -lliquid -lc -lm -lcorrect
clean:
	rm ./a.out