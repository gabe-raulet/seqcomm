#include "mpiutil.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

void mpi_info(MPI_Comm comm, int *myrank, int *nprocs)
{
    int flag;
    MPI_Initialized(&flag);

    if (!flag)
    {
        fprintf(stderr, "mpi_info cannot be called before MPI has been initialized [fatal]\n");
        exit(-1);
    }

    if (myrank) MPI_Comm_rank(comm, myrank);
    if (nprocs) MPI_Comm_size(comm, nprocs);
}

int mpi_get_myrank(MPI_Comm comm)
{
    int myrank;
    mpi_info(comm, &myrank, NULL);
    return myrank;
}

int mpi_get_nprocs(MPI_Comm comm)
{
    int nprocs;
    mpi_info(comm, NULL, &nprocs);
    return nprocs;
}

void mpi_fatal_error(int err, const char *file, const char *func, int line)
{
    int myrank, nprocs, len;
    char msg[MPI_MAX_ERROR_STRING] = {0};

    mpi_info(MPI_COMM_WORLD, &myrank, &nprocs);
    MPI_Error_string(err, msg, &len);

    fprintf(stderr, "MPI error ... %s:%d %s (world rank %d/%d)\n", file, line, func, myrank, nprocs);

    MPI_Abort(MPI_COMM_WORLD, -1);
}
