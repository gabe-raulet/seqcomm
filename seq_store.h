#ifndef SEQ_STORE_H_
#define SEQ_STORE_H_

#include "fasta_index.h"

typedef struct
{
    uint8_t *buf; /* encoded sequence buffer (2 bits per nucleotide) */
    size_t *lengths; /* sequence lengths */
    size_t *offsets; /* sequence buffer offsets */
    size_t avail;   /* length/offsets allocated */
    size_t numbytes; /* buffer length */
    size_t numseqs;  /* number of sequences */
    size_t totbases; /* total number of nucleotides stored */
} seq_store_t;

int seq_store_read(seq_store_t *store, char const *fname, const fasta_index_t faidx);
int seq_store_free(seq_store_t *store);
void seq_store_log(const seq_store_t store, char const *fname_prefix, MPI_Comm comm);
int seq_store_get(const seq_store_t store, size_t id, char **seq);

#endif
