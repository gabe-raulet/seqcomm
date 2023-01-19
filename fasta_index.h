#ifndef FASTA_INDEX_H_
#define FASTA_INDEX_H_

#include "mpiutil.h"
#include <stdint.h>

typedef struct { size_t len, pos, bases; } fasta_record_t;

typedef struct
{
    commgrid_t const *grid;
    fasta_record_t *records;
    size_t num_records;
} fasta_index_t;

int fasta_index_read(fasta_index_t *faidx, char const *fname, commgrid_t const *grid);
int fasta_index_free(fasta_index_t *faidx);
void fasta_index_log(const fasta_index_t faidx, char const *fname_prefix);

#endif
