#include "elba_error.h"

static int elba_success(void) { return ELBA_SUCCESS; }
static int elba_terminate(void) { return ELBA_TERMINATE; }
static int elba_failure(void) { return ELBA_FAILURE; }
static int elba_other(void) { return 999; }


int main(int argc, char *argv[])
{
    int myrank, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    ELBA_CHECK(elba_success());
    ELBA_CHECK(elba_other());

    ELBA_CHECK(elba_terminate());
    /* ELBA_CHECK(elba_failure()); */

    MPI_Finalize();
    return 0;
}
