default: rom-parser rom-fixer

rom-parser: rom-parser.c
	gcc -o rom-parser rom-parser.c

rom-fixer: rom-parser.c
	gcc -DFIXER -o rom-fixer rom-parser.c

clean:
	rm -f rom-parser rom-fixer
