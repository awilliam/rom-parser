default: rom-parser

rom-parser: rom-parser.c
	gcc -o rom-parser rom-parser.c

clean:
	rm -f rom-parser
