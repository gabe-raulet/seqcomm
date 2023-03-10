CC=mpicc
#FLAGS=-g -O0 -fsanitize=address -fno-omit-frame-pointer -Wall
FLAGS=-O2 -Wall

all: main

mpiutil.o: mpiutil.c mpiutil.h
	$(CC) $(FLAGS) -c -o mpiutil.o mpiutil.c -lm

seq_store.o: seq_store.c seq_store.h
	$(CC) $(FLAGS) -c -o seq_store.o seq_store.c -lm

fasta_index.o: fasta_index.c fasta_index.h
	$(CC) $(FLAGS) -c -o fasta_index.o fasta_index.c -lm

mstring.o: mstring.c mstring.h
	$(CC) $(FLAGS) -c -o mstring.o mstring.c -lm

main.o: main.c
	$(CC) $(FLAGS) -c -o main.o main.c -lm

main: main.o fasta_index.o mpiutil.o seq_store.o mstring.o
	$(CC) $(FLAGS) -o $@ $^ -lm

clean:
	rm -rf *.o *.dSYM *.log
