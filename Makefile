csvcut: csvcut.c
	gcc -O2 csvcut.c -o csvcut
	strip csvcut

clean:
	rm csvcut
