#################################################
#	Projekt: Projekt do p�edm�tu POS			#
#		 Ticket Algorithm						#
#	 Autor: Bc. Jakub Stejskal <xstejs24>		#
# Nazev souboru: GNUakefile						#	
#	  Datum: 9. 2. 2017		    				#		
#	  Verze: 1.0								#
#################################################

CC = gcc 
CFLAGS = -std=gnu99 -Wall -pedantic -W -Wextra -pthread -D_SVID_SOURCE -D_GNU_SOURCE -D_BSD_SOURCE
LOGIN = xstejs24
PROJ_NAME = proj02
FILES = proj02.o 
PACK = *.c *.h Makefile

default: proj01.o
	$(CC) $(CFLAGS) proj01.c -o $(PROJ_NAME) $^ -lrt	
	
pack: clean
	rm -f $(LOGIN).zip
	zip -r $(LOGIN).zip $(PACK)
	
run:
	./$(PROJ_NAME)
	
clean:
	rm -f *.o *.out $(PROJ_NAME)
