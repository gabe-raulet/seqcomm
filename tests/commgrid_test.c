#include "commgrid.h"
#include "elba_error.h"

int main(int argc, char *argv[])
{
    int myrank, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    commgrid_t grid;
    ELBA_CHECK(commgrid_init(&grid, MPI_COMM_WORLD));
    commgrid_log(grid, stderr);
    ELBA_CHECK(commgrid_free(&grid));

    MPI_Finalize();
    return 0;
}
