#ifndef ELBA_H_
#define ELBA_H_

#include <mpi.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>

/*
 * ELBA parameters structure. Stores all the user-specified
 * (or default) parameters used.
 */
typedef struct
{
    int kmer_size;
    int lower_kmer_bound;
    int upper_kmer_bound;
    int xdrop;
    int mat;
    int mis;
    int gap;
    char *target_fname;
    char *query_fname;
    char const *output_fname;
} elba_opt_t;

/*
 * Initializes default parameters.
 */
void elba_opt_init(elba_opt_t *opt);

/*
 * Parses user-specified parameters from the command line.
 */
int elba_opt_parse(elba_opt_t *opt, int myrank, int argc, char *argv[]);

/*
 * Prints out command-line interface usage.
 */
int elba_opt_usage(elba_opt_t const opt, int myrank, char const *prog);


/*
 * Logs parameters that will be used for current execution cycle.
 */
int elba_opt_log(elba_opt_t const opt, int myrank, FILE *f);

#endif
