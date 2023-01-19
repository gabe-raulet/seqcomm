#ifndef MPI_UTIL_H_
#define MPI_UTIL_H_

#include <mpi.h>
#include <stdio.h>

typedef struct
{
    int dims;

    MPI_Comm grid_world;
    MPI_Comm row_world;
    MPI_Comm col_world;

    int gridrank;
    int gridrow;
    int gridcol;

} commgrid_t;

int commgrid_init(commgrid_t *grid);
int commgrid_free(commgrid_t *grid);
int commgrid_log(const commgrid_t grid, FILE *f);

void mpi_info(MPI_Comm comm, int *myrank, int *nprocs);
void mpi_fatal_error(int err, const char *file, const char *func, int line);

#define MPI_CHECK(fn)                                        \
    do {                                                     \
        int _err = (fn);                                     \
        if (_err != MPI_SUCCESS) {                           \
            mpi_fatal_error(_err, __FILE__, #fn, __LINE__);  \
        }                                                    \
    } while (0)


#endif
