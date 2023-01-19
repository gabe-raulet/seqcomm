CC=mpicc
FLAGS=-g -O0 -fsanitize=address -fno-omit-frame-pointer -Wall -Werror -pedantic

all: main

mpiutil.o: mpiutil.c mpiutil.h
	$(CC) $(FLAGS) -c -o mpiutil.o mpiutil.c

seq_store.o: seq_store.c seq_store.h
	$(CC) $(FLAGS) -c -o seq_store.o seq_store.c

fasta_index.o: fasta_index.c fasta_index.h
	$(CC) $(FLAGS) -c -o fasta_index.o fasta_index.c

main.o: main.c
	$(CC) $(FLAGS) -c -o main.o main.c

main: main.o fasta_index.o mpiutil.o
	$(CC) $(FLAGS) -o $@ $^

clean:
	rm -rf *.o *.dSYM