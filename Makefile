CC=gcc
CFLAGS=-Wall -Wextra -g

EXEC=gestionnaire joueurH joueurR

all: $(EXEC)

gestionnaire: gestionnaire.c commun.h
	$(CC) $(CFLAGS) -o gestionnaire gestionnaire.c

joueurH: joueurH.c commun.h
	$(CC) $(CFLAGS) -o joueurH joueurH.c

joueurR: joueurR.c commun.h
	$(CC) $(CFLAGS) -o joueurR joueurR.c

run: all
	@echo "Lancement de la partie avec 4 joueurs (1 humain, 3 robots)"
	./gestionnaire 4

clean:
	rm -f $(EXEC)



