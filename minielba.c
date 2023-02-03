#include "elba.h"
#include "elba_str.h"
#include "elba_error.h"
#include "elba_faidx.h"
#include "elba_comm.h"
#include "elba_seq_store.h"
#include "elba_str_store.h"
#include "mpiutil.h"
#include "commgrid.h"

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    /*
     * Initialize MPI.
     */
    MPI_Init(&argc, &argv);

    /*
     * Initialize CombBLAS grid communicator.
     */
    commgrid_t grid;
    ELBA_CHECK(commgrid_init(&grid, MPI_COMM_WORLD));
    commgrid_log(grid, stderr);

    /*
     * main() probably will need to know where in the grid it is.
     */
    int myrank, nprocs;
    mpi_info(MPI_COMM_WORLD, &myrank, &nprocs);

    /*
     * Initialize default ELBA parameters.
     */
    elba_opt_t opt;
    elba_opt_init(&opt);

    /*
     * Parse user provided command-line parameters.
     */
    ELBA_CHECK(elba_opt_parse(&opt, myrank, argc, argv));
    elba_opt_log(opt, grid.gridrank, stderr);

    /*
     * Get target and query FASTA/FAIDX file names.
     */
    elba_str_t t_fa_fname, t_fai_fname, q_fa_fname, q_fai_fname;

    t_fa_fname = ELBA_STR_LIT(opt.target_fname);
    q_fa_fname = ELBA_STR_LIT(opt.query_fname);

    t_fai_fname = ELBA_STR_DUP(t_fa_fname);
    q_fai_fname = ELBA_STR_DUP(q_fa_fname);

    ELBA_CHECK(elba_str_cat(&t_fai_fname, ".fai"));
    ELBA_CHECK(elba_str_cat(&q_fai_fname, ".fai"));

    /*
     * Read target and query FAIDX files.
     */
    elba_fasta_index_t t_index, q_index;
    elba_str_store_t t_names, q_names;

    t_names = ELBA_STR_STORE_INIT;
    q_names = ELBA_STR_STORE_INIT;

    ELBA_CHECK(elba_fasta_index_read(&t_index, elba_str_str(t_fai_fname), &t_names, &grid));
    ELBA_CHECK(elba_fasta_index_read(&q_index, elba_str_str(q_fai_fname), &q_names, &grid));

    elba_fasta_index_log(t_index, elba_str_str(t_fai_fname), stderr);
    elba_fasta_index_log(q_index, elba_str_str(q_fai_fname), stderr);

    /*
     * Free memory.
     */
    ELBA_CHECK(elba_str_free(&t_fai_fname));
    ELBA_CHECK(elba_str_free(&q_fai_fname));

    /*
     * Read target and query FASTA files. Sequences will be distributed
     * to each processor according to a linear decomposition.
     */
    elba_seq_store_t t_seqs, q_seqs;

    ELBA_CHECK(elba_seq_store_read(&t_seqs, elba_str_str(t_fa_fname), t_index));
    ELBA_CHECK(elba_seq_store_read(&q_seqs, elba_str_str(q_fa_fname), q_index));

    /*
     * Free memory.
     */
    ELBA_CHECK(elba_fasta_index_free(&t_index));
    ELBA_CHECK(elba_fasta_index_free(&q_index));
    ELBA_CHECK(elba_str_free(&t_fa_fname));
    ELBA_CHECK(elba_str_free(&q_fa_fname));

    /*
     * Broadcast sequence names.
     */
    ELBA_CHECK(elba_comm_str_store_broadcast(&t_names, 0, grid.grid_world));
    ELBA_CHECK(elba_comm_str_store_broadcast(&q_names, 0, grid.grid_world));

    /*
     * Share sequences across processor rows and columns.
     */
    elba_seq_store_t t_row_seqs, t_col_seqs, q_row_seqs, q_col_seqs;

    ELBA_CHECK(elba_seq_store_share(t_seqs, &t_row_seqs, &t_col_seqs, &grid));
    ELBA_CHECK(elba_seq_store_share(q_seqs, &q_row_seqs, &q_col_seqs, &grid));

    /*
     * Free memory.
     */
    ELBA_CHECK(elba_seq_store_free(&t_row_seqs));
    ELBA_CHECK(elba_seq_store_free(&q_row_seqs));

    ELBA_CHECK(elba_seq_store_free(&t_col_seqs));
    ELBA_CHECK(elba_seq_store_free(&q_col_seqs));

    ELBA_CHECK(elba_seq_store_free(&t_seqs));
    ELBA_CHECK(elba_seq_store_free(&q_seqs));

    ELBA_CHECK(commgrid_free(&grid));

    MPI_Finalize();

    return 0;
}
