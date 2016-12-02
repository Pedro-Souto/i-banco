# Makefile, versao 3
# Sistemas Operativos, DEI/IST/ULisboa 2016-17

# CFLAGS=-g -Wall -pedantic
CFLAGS=-g -Wall -pedantic -std=gnu99
#                         -std=gnu99  to allow c++ comments ("//")
CC=gcc

all: i-banco i-banco-terminal

i-banco: i-banco.o commandlinereader.o contas.o
	$(CC) $(CFLAGS) -pthread -o i-banco i-banco.o commandlinereader.o contas.o

i-banco-terminal: i-banco-terminal.o contas.o commandlinereader.o
	$(CC) $(CFLAGS) -pthread -o i-banco-terminal i-banco-terminal.o contas.o commandlinereader.o

i-banco.o: i-banco.c contas.h commandlinereader.h
	$(CC) $(CFLAGS) -c i-banco.c

i-banco-terminal.o: i-banco-terminal.c contas.h commandlinereader.h
	$(CC) $(CFLAGS) -c i-banco-terminal.c

contas.o: contas.c contas.h
	$(CC) $(CFLAGS) -c contas.c

commandlinereader.o: commandlinereader.c commandlinereader.h
	$(CC) $(CFLAGS) -c commandlinereader.c

clean:
	rm -f *.o i-banco
	rm -f *.o i-banco-terminal
	rm -f i-banco-pipe
	rm -f i-banco-pid*
	rm -f log.txt

zip:
	rm -f SO-ex4-solucao-v1.zip
	zip   SO-ex4-solucao-v1.zip  input.txt i-banco.c contas.c contas.h commandlinereader.c commandlinereader.h Makefile i-banco-terminal.c
