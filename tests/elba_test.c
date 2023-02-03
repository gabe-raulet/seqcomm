#include "elba.h"
#include "elba_error.h"
#include <assert.h>

int main(int argc, char *argv[])
{
    /*
     * Initialize and setup MPI.
     */
    int myrank, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    elba_opt_t opt;

    /*
     * Initialize default ELBA parameters.
     */
    elba_opt_init(&opt);

    /*
     * Parse user provided command-line parameters.
     */
    ELBA_CHECK(elba_opt_parse(&opt, myrank, argc, argv));

    if (!myrank)
    {
        printf("kmer_size = %d\n", opt.kmer_size);
        printf("lower_kmer_bound = %d\n", opt.lower_kmer_bound);
        printf("upper_kmer_bound = %d\n", opt.upper_kmer_bound);
        printf("xdrop = %d\n", opt.xdrop);
        printf("mat = %d\n", opt.mat);
        printf("mis = %d\n", opt.mis);
        printf("gap = %d\n", opt.gap);
        printf("target_fname = '%s'\n", opt.target_fname);
        printf("query_fname = '%s'\n", opt.query_fname);
        printf("output_fname = '%s'\n", opt.output_fname);
    }


    MPI_Finalize();
    return 0;
}
