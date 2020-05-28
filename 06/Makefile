all: scan assembler

lex.yy.c: scan.l
	flex -i scan.l

scan: lex.yy.c
	gcc lex.yy.c -o scan

assembler: assembler.c
	gcc -std=c99 assembler.c -o assembler

clean:
	rm -f scan lex.yy.c assembler

