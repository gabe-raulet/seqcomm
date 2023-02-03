#ifndef COMMGRID_H_
#define COMMGRID_H_

#include <mpi.h>
#include <stdio.h> /* FILE */

/*
 * CombBLAS 2D processor grid communicator information.
 */
typedef struct
{
    int dims; /* dimension of square 2D processor grid */

    MPI_Comm grid_world; /* MPI communicator for the entire grid */
    MPI_Comm row_world;  /* MPI communicator for communication between processors in the same row */
    MPI_Comm col_world;  /* MPI communicator for communication between processors in the same column */

    int gridrank; /* my processor's id within the entire grid */
    int gridrow;  /* my processor's id in my processor row */
    int gridcol;  /* my processor's id in my processor column */

} commgrid_t;

/*
 * Initializes a new commgrid_t object. Note that this object
 * is meant to be created once (on each processor due to SPMD)
 * and used for all CombBLAS-related communication.
 *
 * TODO: perhaps I should make this return a const structure...
 */
int commgrid_init(commgrid_t *grid, MPI_Comm comm);

/*
 * Frees a commgrid_t object.
 */
int commgrid_free(commgrid_t *grid);

/*
 * Logs information about about calling processor's
 * coordinates in the processor grid.
 */
int commgrid_log(commgrid_t const grid, FILE *f);

/*
 * Writes a tag indicating grid rank coordinates to char buffer.
 */
int commgrid_tag(commgrid_t const grid, char *tag);

/*
 * Gets the total number of processors in the 2D grid.
 */
#define commgrid_nprocs(grid) ((grid).dims*(grid).dims)

#endif
