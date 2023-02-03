#include "elba_seq_store.h"
#include "elba_error.h"
#include "mpiutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

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

static void push(elba_seq_store_t *store, char *s, size_t len, size_t *avail, size_t id)
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

    if (store->numseqs+1 >= *avail)
    {
        *avail = next_pow2(store->numseqs+1);
        store->lengths = realloc(store->lengths, *avail * sizeof(size_t));
        store->offsets = realloc(store->offsets, *avail * sizeof(size_t));
        store->gids = realloc(store->gids, *avail * sizeof(size_t));
    }

    store->gids[store->numseqs] = id;
    store->lengths[store->numseqs] = len;
    store->offsets[store->numseqs++] = offset;
    store->numbytes += n;
}

int elba_seq_store_read(elba_seq_store_t *store, char const *fname, const elba_fasta_index_t faidx)
{
    if (!store) return ELBA_FAILURE;

    *store = (elba_seq_store_t){0};
    size_t seq_store_avail = 0;
    size_t num_records = faidx.num_records;

    /* first and last records in my local chunk */
    elba_faidx_record_t first_record = faidx.records[0];
    elba_faidx_record_t last_record = faidx.records[num_records-1];

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
        elba_faidx_record_t *record = faidx.records + i;
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

        push(store, seqbuf, record->len, &seq_store_avail, i+offset);
    }

    store->lengths = realloc(store->lengths, store->numseqs * sizeof(size_t));
    store->offsets = realloc(store->offsets, store->numseqs * sizeof(size_t));
    store->gids = realloc(store->gids, store->numseqs * sizeof(size_t));

    free(mychunk);

    return ELBA_SUCCESS;
}

int elba_seq_store_get(elba_seq_store_t const store, size_t lid, size_t *gid, char **seq)
{
    static const char bases[5] = {'A', 'C', 'G', 'T', 'N'};

    if (!seq) return ELBA_FAILURE;

    size_t len = store.lengths[lid];
    size_t offset = store.offsets[lid];

    if (gid) *gid = store.gids[lid];

    char *s = realloc(*seq, len+1);
    if (s) *seq = s;

    memset(s, '\0', len+1);

    for (size_t i = 0; i < len; ++i)
    {
        s[i] = bases[(store.buf[offset + (i/4)] >> ((i%4)<<1))&3];
    }

    return ELBA_SUCCESS;
}

void elba_seq_store_info(elba_seq_store_t const store, char const *fname, commgrid_t const *grid)
{
    int myrank = grid->gridrank;
    char *info;

    int len = asprintf(&info, "myrank=%d\tgridrow=%d\tgridcol=%d\tnumbytes=%lu\tnumseqs=%lu\ttotbases=%lu\n",
                               myrank, grid->gridrow, grid->gridcol, store.numbytes, store.numseqs, store.totbases);

    MPI_File fh;

    MPI_CHECK(MPI_File_open(grid->grid_world, fname, MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL, &fh));
    MPI_CHECK(MPI_File_write_ordered(fh, info, len, MPI_CHAR, MPI_STATUS_IGNORE));
    MPI_CHECK(MPI_File_close(&fh));

    free(info);
}

void elba_seq_store_log(elba_seq_store_t const store, char const *fname_prefix, elba_str_store_t const *names, MPI_Comm comm)
{
    size_t numseqs;
    int myrank;
    char *log_fname;
    FILE *f;

    myrank = mpi_get_myrank(comm);

    numseqs = store.numseqs;
    asprintf(&log_fname, "%s.rank%d.log", fname_prefix, myrank);

    f = fopen(log_fname, "w");
    free(log_fname);

    size_t maxlen;

    ELBA_CHECK(elba_str_store_get_maxlen(*names, &maxlen));

    char *name = names? malloc(maxlen+1) : NULL;

    for (size_t i = 0; i < numseqs; ++i)
    {
        size_t gid;
        char *seq = NULL;
        elba_seq_store_get(store, i, &gid, &seq);

        if (name)
        {
            ELBA_CHECK(elba_str_store_get_strcpy(*names, gid, name));
            fprintf(f, "%s\t%s\n", name, seq);
        }
        else
        {
            fprintf(f, "%lu\t%s\n", gid, seq);
        }

        free(seq);
    }

    if (name) free(name);
}

int elba_seq_store_free(elba_seq_store_t *store)
{
    if (!store) return ELBA_FAILURE;

    free(store->buf);
    free(store->lengths);
    free(store->offsets);
    free(store->gids);

    *store = (elba_seq_store_t){0};

    return ELBA_SUCCESS;
}

static inline void partial_sum(int *displs, int *counts, int n)
{
    displs[0] = 0;

    for (int i = 0; i < n-1; ++i)
        displs[i+1] = displs[i] + counts[i];
}

/* blocking version */
int elba_seq_store_share(elba_seq_store_t const send_store, elba_seq_store_t *row_store, elba_seq_store_t *col_store, commgrid_t const *grid)
{
    /*
     * Note: this function currently is inefficient due to the many collective
     *       MPI calls. At some point it will need to be simplified using derived
     *       datatypes and perhaps smarter packing/unpacking.
     */

    if (!row_store || !col_store || !grid)
        return -1;

    /*
     * Compute row and column reduction on sequence storage's size information,
     * i.e. number of bytes for the buffer, number of sequences stored, and
     * the number of total bases.
     */
    size_t send_info[3], row_info[3], col_info[3];

    send_info[0] = send_store.numbytes;
    send_info[1] = send_store.numseqs;
    send_info[2] = send_store.totbases;

    MPI_Allreduce(send_info, row_info, 3, MPI_SIZE_T, MPI_SUM, grid->row_world);
    MPI_Allreduce(send_info, col_info, 3, MPI_SIZE_T, MPI_SUM, grid->col_world);

    /*
     * Initialize row and column storage structures with their size information.
     */
    *row_store = (elba_seq_store_t){NULL, NULL, NULL, NULL, row_info[0], row_info[1], row_info[2]};
    *col_store = (elba_seq_store_t){NULL, NULL, NULL, NULL, col_info[0], col_info[1], col_info[2]};

    /*
     * Allocate memory for row and column sequence storage.
     */
    row_store->buf = malloc(row_store->numbytes);
    row_store->lengths = malloc(row_store->numseqs * sizeof(size_t));
    row_store->offsets = malloc(row_store->numseqs * sizeof(size_t));
    row_store->gids = malloc(row_store->numseqs * sizeof(size_t));

    col_store->buf = malloc(col_store->numbytes);
    col_store->lengths = malloc(col_store->numseqs * sizeof(size_t));
    col_store->offsets = malloc(col_store->numseqs * sizeof(size_t));
    col_store->gids = malloc(col_store->numseqs * sizeof(size_t));

    int sendcnt = (int)send_store.numseqs;
    size_t row_offset = send_store.numbytes;
    size_t col_offset = send_store.numbytes;

    MPI_Exscan(MPI_IN_PLACE, &row_offset, 1, MPI_SIZE_T, MPI_SUM, grid->row_world);
    if (grid->gridcol == 0) row_offset = 0;

    MPI_Exscan(MPI_IN_PLACE, &col_offset, 1, MPI_SIZE_T, MPI_SUM, grid->col_world);
    if (grid->gridrow == 0) col_offset = 0;

    size_t *row_offsets = malloc(sendcnt * sizeof(size_t));
    size_t *col_offsets = malloc(sendcnt * sizeof(size_t));

    for (int i = 0; i < sendcnt; ++i)
    {
        row_offsets[i] = send_store.offsets[i] + row_offset;
        col_offsets[i] = send_store.offsets[i] + col_offset;
    }

    int *recvcnts = malloc(grid->dims * sizeof(int));
    int *displs = malloc(grid->dims * sizeof(int));

    recvcnts[grid->gridcol] = sendcnt;
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, recvcnts, 1, MPI_INT, grid->row_world);
    partial_sum(displs, recvcnts, grid->dims);

    MPI_Allgatherv(send_store.lengths, sendcnt, MPI_SIZE_T, row_store->lengths, recvcnts, displs, MPI_SIZE_T, grid->row_world);
    MPI_Allgatherv(send_store.gids,    sendcnt, MPI_SIZE_T, row_store->gids,    recvcnts, displs, MPI_SIZE_T, grid->row_world);
    MPI_Allgatherv(row_offsets,        sendcnt, MPI_SIZE_T, row_store->offsets, recvcnts, displs, MPI_SIZE_T, grid->row_world);

    recvcnts[grid->gridrow] = sendcnt;
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, recvcnts, 1, MPI_INT, grid->col_world);
    partial_sum(displs, recvcnts, grid->dims);

    MPI_Allgatherv(send_store.lengths, sendcnt, MPI_SIZE_T, col_store->lengths, recvcnts, displs, MPI_SIZE_T, grid->col_world);
    MPI_Allgatherv(send_store.gids,    sendcnt, MPI_SIZE_T, col_store->gids,    recvcnts, displs, MPI_SIZE_T, grid->col_world);
    MPI_Allgatherv(col_offsets,        sendcnt, MPI_SIZE_T, col_store->offsets, recvcnts, displs, MPI_SIZE_T, grid->col_world);

    free(row_offsets);
    free(col_offsets);

    sendcnt = (int)send_store.numbytes;

    recvcnts[grid->gridcol] = sendcnt;
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, recvcnts, 1, MPI_INT, grid->row_world);
    partial_sum(displs, recvcnts, grid->dims);

    MPI_Allgatherv(send_store.buf, sendcnt, MPI_UINT8_T, row_store->buf, recvcnts, displs, MPI_UINT8_T, grid->row_world);

    recvcnts[grid->gridrow] = sendcnt;
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, recvcnts, 1, MPI_INT, grid->col_world);
    partial_sum(displs, recvcnts, grid->dims);

    MPI_Allgatherv(send_store.buf, sendcnt, MPI_UINT8_T, col_store->buf, recvcnts, displs, MPI_UINT8_T, grid->col_world);

    free(recvcnts);
    free(displs);

    return 0;
}


