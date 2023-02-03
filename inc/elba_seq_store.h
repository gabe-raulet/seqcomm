#ifndef SEQ_STORE_H_
#define SEQ_STORE_H_

#include "elba_str_store.h"
#include "elba_faidx.h"
#include "mpiutil.h"
#include "commgrid.h"

typedef struct
{
    uint8_t *buf; /* encoded sequence buffer (2 bits per nucleotide) */
    size_t *lengths; /* sequence lengths */
    size_t *offsets; /* sequence buffer offsets */
    size_t *gids;    /* global sequence ids */
    size_t numbytes; /* buffer length */
    size_t numseqs;  /* number of sequences */
    size_t totbases; /* total number of nucleotides stored */
} elba_seq_store_t;

int elba_seq_store_read(elba_seq_store_t *store, char const *fname, const elba_fasta_index_t faidx);
int elba_seq_store_free(elba_seq_store_t *store);
void elba_seq_store_info(elba_seq_store_t const store, char const *fname, commgrid_t const *grid);
void elba_seq_store_log(elba_seq_store_t const store, char const *fname_prefix, elba_str_store_t const *names, MPI_Comm comm);
int elba_seq_store_share(elba_seq_store_t const send_store, elba_seq_store_t *row_store, elba_seq_store_t *col_store, commgrid_t const *grid);
int elba_seq_store_get(elba_seq_store_t const store, size_t lid, size_t *gid, char **seq);

#endif
