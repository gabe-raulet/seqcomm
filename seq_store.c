#include "seq_store.h"
#include "mpiutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

const uint8_t nt4map[256] =
{
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,

    4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 0, 4,
    4, 4, 4, 4,  3, 3, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 0, 4,
    4, 4, 4, 4,  3, 3, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,

    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,

    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

//typedef struct
//{
//    uint8_t *buf; /* encoded sequence buffer (2 bits per nucleotide) */
//    size_t *lengths; /* sequence lengths */
//    size_t *offsets; /* sequence buffer offsets */
//    size_t avail;
//    size_t numbytes; /* buffer length */
//    size_t numseqs;  /* number of sequences */
//    size_t totbases; /* total number of nucleotides stored */
//} seq_store_t;

static void push(seq_store_t *store, char *s, size_t len)
{
    size_t n = (len + 3) / 4;
    size_t offset = store->numbytes;

    store->buf = realloc(store->buf, offset + n);

    memset(store->buf + offset, 0, n);

    for (size_t i = 0; i < len; ++i)
    {
        store->buf[offset + (i/4)] |= nt4map[(int)s[i]] << ((i%4)<<1);
    }

    store->totbases += len;

    if (store->numseqs+1 >= store->avail)
    {
        store->avail = up_size_t(store->numseqs+1);
        store->lengths = realloc(store->lengths, store->avail * sizeof(size_t));
        store->offsets = realloc(store->offsets, store->avail * sizeof(size_t));
    }

    store->lengths[store->numseqs] = len;
    store->offsets[store->numseqs++] = offset;
    store->numbytes += n;
}

int seq_store_read(seq_store_t *store, const char *fname, const fasta_index_t faidx)
{
    if (!store) return -1;

    *store = (seq_store_t){0};

    /* int myrank = faidx.grid->gridrank; */
    /* int nprocs = faidx.grid->dims * faidx.grid->dims; */
    size_t num_records = faidx.num_records;

    /* first and last records in my local chunk */
    fasta_record_t first_record = faidx.records[0];
    fasta_record_t last_record = faidx.records[num_records-1];

    /* position of first and last (exclusive) character within FASTA file that my chunk needs */
    MPI_Offset startpos = first_record.pos;
    MPI_Offset endpos = last_record.pos + last_record.len + (last_record.len / last_record.bases);

    MPI_File fh;
    MPI_CHECK(MPI_File_open(faidx.grid->grid_world, fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh));

    MPI_Offset filesize;
    MPI_CHECK(MPI_File_get_size(fh, &filesize));

    /* last process rank may need to adjust end position if the FASTA file isn't terminated with a '\n' */
    endpos = endpos < filesize? endpos : filesize;

    int mychunksize = endpos - startpos;
    char *mychunk = malloc(mychunksize);

    MPI_CHECK(MPI_File_read_at_all(fh, startpos, mychunk, mychunksize, MPI_CHAR, MPI_STATUS_IGNORE));
    MPI_CHECK(MPI_File_close(&fh));

    size_t maxlen = 0;

    for (size_t i = 0; i < num_records; ++i)
    {
        maxlen = maxlen > faidx.records[i].len? maxlen : faidx.records[i].len;
    }

    size_t offset = 0;
    MPI_Exscan(&num_records, &offset, 1, MPI_SIZE_T, MPI_SUM, faidx.grid->grid_world);

    char *seqbuf = malloc(maxlen);

    for (size_t i = 0; i < num_records; ++i)
    {
        fasta_record_t *record = faidx.records + i;
        size_t bases = record->bases;
        size_t locpos = 0;
        ptrdiff_t chunkpos = record->pos - startpos;
        ptrdiff_t remain = record->len;
        char *bufptr = seqbuf;

        while (remain > 0)
        {
            size_t cnt = bases < remain? bases : remain;
            memcpy(bufptr, mychunk + chunkpos + locpos, cnt);
            bufptr += cnt;
            remain -= cnt;
            locpos += (cnt+1);
        }

        push(store, seqbuf, record->len);
    }

    free(mychunk);
    return 0;
}

int seq_store_get(const seq_store_t store, size_t id, char **seq)
{
    static const char bases[5] = {'A', 'C', 'G', 'T', 'N'};

    if (!seq) return -1;

    size_t len = store.lengths[id];
    size_t offset = store.offsets[id];

    char *s = realloc(*seq, len+1);
    if (s) *seq = s;

    memset(s, '\0', len+1);

    for (size_t i = 0; i < len; ++i)
    {
        s[i] = bases[(store.buf[offset + (i/4)] >> ((i%4)<<1))&3];
    }

    return len <= INT_MAX? len : INT_MAX;
}

void seq_store_log(const seq_store_t store, char const *fname_prefix, MPI_Comm comm)
{
    size_t numseqs;
    int myrank;
    char *log_fname;
    FILE *f;

    mpi_info(comm, &myrank, NULL);

    numseqs = store.numseqs;
    asprintf(&log_fname, "%s.rank%d.log", fname_prefix, myrank);

    f = fopen(log_fname, "w");
    free(log_fname);

    size_t offset;
    MPI_Exscan(&numseqs, &offset, 1, MPI_SIZE_T, MPI_SUM, comm);
    if (!myrank) offset = 0;

    for (size_t i = 0; i < numseqs; ++i)
    {
        char *seq = NULL;
        seq_store_get(store, i, &seq);
        fprintf(f, "%lu\t%s\n", offset+i, seq);
        free(seq);
    }
}

int seq_store_free(seq_store_t *store)
{
    if (!store) return -1;

    free(store->buf);
    free(store->lengths);
    free(store->offsets);
    *store = (seq_store_t){0};

    return 0;
}
