#include "commgrid.h"
#include "elba_error.h" // ELBA error codes
#include <math.h>       // sqrt
#include <assert.h>

int commgrid_init(commgrid_t *grid, MPI_Comm comm)
{
    double ddims;
    int myrank, nprocs, dims;

    if (!grid) return ELBA_FAILURE;

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &myrank);

    ddims = sqrt(nprocs);
    dims = (int) ddims;

    if (dims*dims != nprocs)
    {
        if (!myrank)
            fprintf(stderr, "commgrid_init was passed a communicator with %d processors, which is not a perfect square\n", nprocs);

        return ELBA_TERMINATE;
    }

    grid->dims = dims;
    grid->gridrank = myrank;
    grid->gridrow = myrank / dims;
    grid->gridcol = myrank % dims;

    MPI_Comm_dup(MPI_COMM_WORLD, &grid->grid_world);
    MPI_Comm_split(grid->grid_world, grid->gridrow, grid->gridrank, &grid->row_world);
    MPI_Comm_split(grid->grid_world, grid->gridcol, grid->gridrank, &grid->col_world);

    int rowrank, colrank;
    MPI_Comm_rank(grid->row_world, &rowrank);
    MPI_Comm_rank(grid->col_world, &colrank);

    assert((rowrank == grid->gridcol));
    assert((colrank == grid->gridrow));

    return ELBA_SUCCESS;
}

int commgrid_free(commgrid_t *grid)
{
    if (!grid) return ELBA_FAILURE;

    MPI_Comm_free(&grid->grid_world);
    MPI_Comm_free(&grid->row_world);
    MPI_Comm_free(&grid->col_world);

    *grid = (commgrid_t){0};

    return ELBA_SUCCESS;
}

int commgrid_log(commgrid_t const grid, FILE *f)
{
    if (!grid.gridrank)
    {
        fprintf(f, "commgrid(%d x %d)\n", grid.dims, grid.dims);
        fflush(f);
    }

    return ELBA_SUCCESS;
}

int commgrid_tag(commgrid_t const grid, char *tag)
{
    sprintf(tag, "P(%d)==P(%d,%d)", grid.gridrank, grid.gridrow, grid.gridcol);
    return ELBA_SUCCESS;
}
