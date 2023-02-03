#include "mpiutil.h"
#include <assert.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int myrank, nprocs;

    mpi_timer_t timer;
    start_mpi_timer(timer);

    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    assert(myrank == mpi_get_myrank(MPI_COMM_WORLD));
    assert(nprocs == mpi_get_nprocs(MPI_COMM_WORLD));

    mpi_info(MPI_COMM_WORLD, &myrank, &nprocs);
    assert(myrank == mpi_get_myrank(MPI_COMM_WORLD));
    assert(nprocs == mpi_get_nprocs(MPI_COMM_WORLD));


    stop_mpi_timer(timer);
    print_mpi_timer(timer, this is how long it took);


    MPI_Finalize();
    return 0;
}
