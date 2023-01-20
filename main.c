#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mpiutil.h"
#include "mstring.h"
#include "fasta_index.h"
#include "seq_store.h"

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

#define get_faidx_fname(fasta_fname, faidx_fname) \
    do { \
        (faidx_fname) = alloca(strlen((fasta_fname)) + 5); \
        sprintf((faidx_fname), "%s.fai", (fasta_fname)); \
    } while (0)

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    const char *fasta_fname = argv[1];
    char *faidx_fname;
    get_faidx_fname(fasta_fname, faidx_fname);

    commgrid_t grid;
    assert(commgrid_init(&grid) != -1);

    fasta_index_t faidx;
    string_store_t *names_ptr;

#ifdef USE_NAMES
    string_store_t names = STRING_STORE_INIT;
    names_ptr = &names;
#else
    names_ptr = NULL;
#endif

    fasta_index_read(&faidx, faidx_fname, names_ptr, &grid);

#ifdef USE_NAMES
    sstore_mpi_bcast(names_ptr, 0, grid.grid_world);
#endif

    seq_store_t store;
    seq_store_read(&store, fasta_fname, faidx);
    seq_store_log(store, "orig_store", names_ptr, grid.grid_world);

    fasta_index_free(&faidx);

    seq_store_t row_store, col_store;
    seq_store_share(store, &row_store, &col_store, &grid);

    seq_store_log(row_store, "row_store", names_ptr, grid.grid_world);
    seq_store_log(col_store, "col_store", names_ptr, grid.grid_world);

#ifdef USE_NAMES
    string_store_destroy(names);
#endif

    seq_store_free(&store);
    seq_store_free(&row_store);
    seq_store_free(&col_store);
    commgrid_free(&grid);

    MPI_Finalize();
    return 0;
}
