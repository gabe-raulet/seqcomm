#ifndef MPI_UTIL_H_
#define MPI_UTIL_H_

#include <mpi.h>
#include <stdio.h>
#include "size.h" // size_t next_pow2 rounding and "uint64_t ~ unsigned long" assertion

/*
 * Make MPI_SIZE_T an alias to the correct MPI_Datatype for size_t.
 */
#ifndef MPI_SIZE_T
#define MPI_SIZE_T MPI_UNSIGNED_LONG
#endif

/*
 * Returns processor id and number of processors within the
 * given MPI communicator. An alternative to MPI_Comm_{rank,size}.
 */
void mpi_info(MPI_Comm comm, int *myrank, int *nprocs);

/*
 * Same as above, but when you want one or the other (also uses return value).
 */
int mpi_get_myrank(MPI_Comm comm);
int mpi_get_nprocs(MPI_Comm comm);

/*
 * MPI timer structure.
 */
typedef struct
{
    double avgtime;
    double mintime;
    double maxtime;
    double _mytime; // "private"
} mpi_timer_t;

/*
 * Starts an MPI timer.
 */
#define start_mpi_timer(timer)                                                                 \
{                                                                                              \
    MPI_Barrier(MPI_COMM_WORLD);                                                               \
    (timer)._mytime = 0.0 - MPI_Wtime();                                                       \
}

/* Stops an MPI timer and then computes the avg, min, and max
 * number of seconds (across all processors in MPI_COMM_WORLD)
 * taken since the timer was last started.
 */
#define stop_mpi_timer(timer)                                                                  \
{                                                                                              \
    (timer)._mytime += MPI_Wtime();                                                            \
    int _myrank, _nprocs;                                                                      \
    mpi_info(MPI_COMM_WORLD, &_myrank, &_nprocs);                                              \
    MPI_Reduce(&(timer)._mytime, &(timer).maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); \
    MPI_Reduce(&(timer)._mytime, &(timer).mintime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD); \
    MPI_Reduce(&(timer)._mytime, &(timer).avgtime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); \
    if (_myrank == 0) (timer).avgtime /= _nprocs;                                              \
}

/*
 * Prints out the last stored MPI timer stats to stderr.
 */
#define print_mpi_timer(timer, label)                                                          \
{                                                                                              \
    if (mpi_get_myrank(MPI_COMM_WORLD) == 0)                                                   \
        fprintf(stderr, "MPI timer '%s' ... avg=%.4f min=%.4f max=%.4f [seconds]\n",           \
                        #label, (timer).avgtime, (timer).mintime, (timer).maxtime);            \
}

/*
 * Error handles MPI Library calls.
 */
#define MPI_CHECK(fn)                                                                          \
{                                                                                              \
    int _err = (fn);                                                                           \
    if (_err != MPI_SUCCESS) mpi_fatal_error(_err, __FILE__, #fn, __LINE__);                   \
}

void mpi_fatal_error(int err, const char *file, const char *func, int line);

#endif
