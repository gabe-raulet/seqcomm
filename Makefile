CC=mpicc
CFLAGS=-Wall
INCLUDE=-I./inc
IGNORE_UNUSED=-Wno-unused-function

D?=0

ifeq ($(D), 1)
CFLAGS+=-g -O0 -fsanitize=address -fno-omit-frame-pointer -Wall
else
CFLAGS+=-O2
endif

TESTS=commgrid_test mpiutil_test elba_error_test elba_str_test elba_str_store_test elba_faidx_test \
	  elba_seq_store_test

OBJS=mpiutil.o commgrid.o elba.o elba_str.o elba_str_store.o elba_seq_store.o elba_faidx.o commgrid.o elba_comm.o

all: minielba
test: $(TESTS)

minielba: minielba.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

%.test: tests/%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(IGNORE_UNUSED) -o $@ $^

mpiutil.o: src/mpiutil.c inc/mpiutil.h inc/size.h
commgrid.o: src/commgrid.c inc/commgrid.h inc/elba_error.h
elba.o: src/elba.c inc/elba.h inc/elba_error.h
elba_str.o: src/elba_str.c inc/elba_str.h inc/elba_error.h inc/size.h
elba_str_store.o: src/elba_str_store.c inc/elba_str_store.h inc/elba_error.h
elba_seq_store.o: src/elba_seq_store.c inc/elba_seq_store.h inc/elba_error.h
elba_faidx.o: src/elba_faidx.c inc/elba_faidx.h inc/elba_error.h
elba_comm.o: src/elba_comm.c inc/elba_comm.h inc/elba_error.h

elba_error.test: tests/elba_error_test.c
commgrid.test: tests/commgrid_test.c commgrid.o
mpiutil.test: tests/mpiutil_test.c mpiutil.o
elba_str.test: tests/elba_str_test.c elba_str.o
elba_str_store.test: tests/elba_str_store_test.c elba_str_store.o elba_str.o
elba_seq_store.test: tests/elba_seq_store_test.c elba_seq_store.o elba_faidx.o elba_str.o elba_str_store.o mpiutil.o elba_comm.o commgrid.o
elba_faidx.test: tests/elba_faidx_test.c elba_faidx.o elba_str.o elba_str_store.o commgrid.o mpiutil.o elba_comm.o

clean:
	rm -rf minielba $(TESTS) *.o *.dSYM
