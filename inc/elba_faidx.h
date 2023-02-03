#ifndef ELBA_FASTA_INDEX_H_
#define ELBA_FASTA_INDEX_H_

#include "commgrid.h"
#include "elba_str_store.h"
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    size_t len;
    size_t pos;
    size_t bases;
} elba_faidx_record_t;

typedef struct
{
    commgrid_t const *grid;
    elba_faidx_record_t *records;
    size_t num_records;
} elba_fasta_index_t;

int elba_fasta_index_read(elba_fasta_index_t *index, char const *fname, elba_str_store_t *names, commgrid_t const *grid);
int elba_fasta_index_free(elba_fasta_index_t *index);
int elba_fasta_index_log(elba_fasta_index_t const index, char const *fname, FILE *f);

#endif

