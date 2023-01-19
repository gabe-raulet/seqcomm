#ifndef MPI_UTIL_H_
#define MPI_UTIL_H_

#include <mpi.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>

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

/*
 * Make MPI_SIZE_T an alias to the correct MPI_Datatype for size_t.
 */
#ifndef MPI_SIZE_T
#if SIZE_MAX == ULONG_MAX
#define MPI_SIZE_T MPI_UNSIGNED_LONG
#else
#error "size_t must be unsigned long"
#endif
#endif

/*
 * Get the next power-of-two greater than or equal to x.
 */

static inline size_t up_size_t(size_t x)
{
    if (x != 0)
        x--;

    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    x |= x>>32;

    return x+1;
}

#endif
