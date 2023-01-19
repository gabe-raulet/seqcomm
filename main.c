#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mpiutil.h"

/*
 * 1. Master process reads .fai file and parses each line into a fasta record.
 *
 * 2. Master process scatters FASTA records to each process in 2D grid, using
 *    linear decomposition.
 *
 * 3. Each process in parallel reads in its sequences from the FASTA (using collective I/O)
 *    and compresses the sequences into a storage buffer.
 *
 * 4. A collective Allgather across the rows of the 2D grid occurs with the storage buffers
 *    on each process being exchanged.
 *
 * 5. Step 4 is repeated using the columns.
 *
 * 6. Unpack/decompress local sequences.
 *
 * 7. At this point, every process should have access to the sequence info it needs
 *    in order to use CombBLAS.
 *
 */

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    commgrid_t grid;
    assert(commgrid_init(&grid) != -1);

    commgrid_log(grid, stderr);
    commgrid_free(&grid);

    MPI_Finalize();
    return 0;
}
