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
        fprintf(stderr, "mpi_info_error: mpi_info cannot be called before MPI has been initialized [fatal]\n");
        exit(-1);
    }

    MPI_Comm_rank(comm, myrank);
    MPI_Comm_size(comm, nprocs);
}

void mpi_fatal_error(int err, const char *file, const char *func, int line)
{
    int myrank, nprocs, len;
    char msg[MPI_MAX_ERROR_STRING] = {0};

    mpi_info(MPI_COMM_WORLD, &myrank, &nprocs);
    MPI_Error_string(err, msg, &len);

    fprintf(stderr, "mpi_fatal_error: %s:%d %s (world rank %d/%d)\n", file, line, func, myrank, nprocs);

    MPI_Abort(MPI_COMM_WORLD, -1);
}


int commgrid_init(commgrid_t *commgrid)
{
    double ddims;
    int myrank, nprocs, dims;

    if (!commgrid) return -1;

    mpi_info(MPI_COMM_WORLD, &myrank, &nprocs);

    ddims = sqrt(nprocs+0.0);
    dims = (int) ddims;

    if (dims*dims != nprocs)
    {
        if (!myrank)
            fprintf(stderr, "commgrid_init_error: MPI_COMM_WORLD has %d processors, which is not a perfect square\n", nprocs);

        MPI_Finalize();
        exit(0);
    }

    commgrid->dims = dims;
    commgrid->gridrank = myrank;
    commgrid->gridrow = myrank / dims;
    commgrid->gridcol = myrank % dims;

    MPI_Comm_dup(MPI_COMM_WORLD, &commgrid->grid_world);
    MPI_Comm_split(commgrid->grid_world, commgrid->gridrow, commgrid->gridrank, &commgrid->row_world);
    MPI_Comm_split(commgrid->grid_world, commgrid->gridcol, commgrid->gridrank, &commgrid->col_world);

    int rowrank, colrank;
    MPI_Comm_rank(commgrid->row_world, &rowrank);
    MPI_Comm_rank(commgrid->col_world, &colrank);

    assert((rowrank == commgrid->gridcol));
    assert((colrank == commgrid->gridrow));

    return 0;
}

int commgrid_free(commgrid_t *commgrid)
{
    if (!commgrid) return -1;

    MPI_Comm_free(&commgrid->grid_world);
    MPI_Comm_free(&commgrid->row_world);
    MPI_Comm_free(&commgrid->col_world);

    *commgrid = (commgrid_t){0};

    return 0;
}

int commgrid_log(const commgrid_t grid, FILE *f)
{
    fprintf(f, "commgrid_log:\n");
    fprintf(f, "\tdimensions (%d x %d) = %d processors\n", grid.dims, grid.dims, grid.dims*grid.dims);
    fprintf(f, "\tgridrank = %d\n\tgridrow = %d\n\tgridcol = %d\n", grid.gridrank, grid.gridrow, grid.gridcol);
    fflush(f);

    return 0;
}

