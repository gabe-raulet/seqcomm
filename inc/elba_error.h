#ifndef ELBA_ERROR_H_
#define ELBA_ERROR_H_

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * ELBA error codes.
 */
#define ELBA_TERMINATE -2 /* program use error, terminate gracefully */
#define ELBA_FAILURE   -1 /* fatal failure, terminate less gracefully */
#define ELBA_SUCCESS    0 /* everything seems okay */

__attribute__ ((unused)) static void elba_error(int err, char const *file, char const *func, int line)
{
    int myrank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if      (err == ELBA_SUCCESS) return;
    else if (err == ELBA_TERMINATE)
    {
        if (myrank == 0) fprintf(stderr, "ELBA terminating ... %s:%d:%s\n", file, line, func);
        MPI_Finalize();
        exit(0);
    }
    else if (err == ELBA_FAILURE)
    {
        if (myrank == 0) fprintf(stderr, "ELBA failure ... %s:%d:%s\n", file, line, func);
        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(-1);
    }
}

/*
 * Checks and handles functions using ELBA error code return values.
 */
#define ELBA_CHECK(fn)  elba_error((fn), __FILE__, #fn, __LINE__)

#endif
