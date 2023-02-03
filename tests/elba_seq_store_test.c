#include "elba_seq_store.h"
#include "elba_str.h"
#include "elba_error.h"
#include "elba_faidx.h"
#include "commgrid.h"
#include "elba_comm.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    commgrid_t grid;
    ELBA_CHECK(commgrid_init(&grid, MPI_COMM_WORLD));

    elba_str_t fasta_fname = ELBA_STR_LIT(argv[1]);
    elba_str_t faidx_fname = ELBA_STR_INIT;

    elba_str_copy(&faidx_fname, &fasta_fname);
    elba_str_cat(&faidx_fname, ".fai");

    elba_fasta_index_t index;
    elba_seq_store_t seqs, rowseqs, colseqs;
    elba_str_store_t names = ELBA_STR_STORE_INIT;

    ELBA_CHECK(elba_fasta_index_read(&index, elba_str_str(faidx_fname), &names, &grid));
    ELBA_CHECK(elba_seq_store_read(&seqs, elba_str_str(fasta_fname), index));
    ELBA_CHECK(elba_comm_str_store_broadcast(&names, 0, grid.grid_world));

    elba_seq_store_log(seqs, "seqs", &names, MPI_COMM_WORLD);

    elba_seq_store_share(seqs, &rowseqs, &colseqs, &grid);

    elba_seq_store_log(rowseqs, "rowseqs", &names, MPI_COMM_WORLD);
    elba_seq_store_log(colseqs, "colseqs", &names, MPI_COMM_WORLD);

    ELBA_CHECK(elba_fasta_index_free(&index));
    ELBA_CHECK(elba_str_store_free(&names));
    ELBA_CHECK(elba_seq_store_free(&seqs));
    ELBA_CHECK(elba_seq_store_free(&rowseqs));
    ELBA_CHECK(elba_seq_store_free(&colseqs));
    ELBA_CHECK(elba_str_free(&fasta_fname));
    ELBA_CHECK(elba_str_free(&faidx_fname));

    MPI_Finalize();
    return 0;
}
