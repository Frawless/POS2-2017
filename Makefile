#################################################
#	Projekt: Projekt do p�edm�tu POS			#
#		 Ticket Algorithm						#
#	 Autor: Bc. Jakub Stejskal <xstejs24>		#
# Nazev souboru: GNUakefile						#	
#	  Datum: 9. 2. 2017		    				#		
#	  Verze: 1.0								#
#################################################

CC = gcc 
CFLAGS = -std=gnu99 -Wall -pedantic -Wextra -g3 -O -pthread
LOGIN = xstejs24
PROJ_NAME = proj02
FILES = proj02.c
PACK = *.c *.h Makefile
	
default: $(FILES)
	$(CC) $(CFLAGS) -o $(PROJ_NAME) $(FILES)
	
pack: clean
	rm -f $(LOGIN).zip
	zip -r $(LOGIN).zip $(PACK)
	
run:
	./$(PROJ_NAME)
	
clean:
	rm -f *.o *.out $(PROJ_NAME)
