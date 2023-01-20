#include "mstring.h"
#include <limits.h>

#ifndef MPI_SIZE_T
#if SIZE_MAX == ULONG_MAX
#define MPI_SIZE_T MPI_UNSIGNED_LONG
#else
#error "size_t must be unsigned long"
#endif
#endif

#define up2(x) ((x)=((x)<=1)?2:(x)-1, (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, (x)|=(x)>>32, (x)++)

int string_vprintf_prv(string_t *buf, int truncate, const char *format, va_list args)
{
    char *s, *dest;
    int size;
    size_t needed;

    size = vasprintf(&s, format, args);
    needed = size + 1 + (truncate? 0 : buf->len);

    if (needed > buf->avail)
    {
        up2(needed);
        buf->avail = needed;
        buf->buf = realloc(buf->buf, buf->avail);
    }

    dest = buf->buf + (truncate? 0 : buf->len);

    memcpy(dest, s, size);

    buf->len = truncate? size : buf->len + size;
    buf->buf[buf->len] = 0;

    return size;
}

int string_printf(string_t *buf, int truncate, const char *format, ...)
{
    int size;
    va_list args;
    va_start(args, format);
    size = string_vprintf_prv(buf, truncate, format, args);
    va_end(args);
    return size;
}

void string_pad_null(string_t *buf)
{
    size_t needed = buf->len+1;

    if (buf->avail >= needed && buf->buf[buf->len] == '\0')
        return;
    else if (needed > buf->avail)
    {
        up2(needed);
        buf->avail = needed;
        buf->buf = realloc(buf->buf, buf->avail);
    }

    buf->buf[buf->len++] = '\0';
}

int string_push(string_t *buf, char *s, size_t len, int keep_null)
{
    size_t needed = buf->len + len + 1 + !!(keep_null);

    if (needed > buf->avail)
    {
        up2(needed);
        buf->avail = needed;
        buf->buf = realloc(buf->buf, buf->avail);
    }

    memcpy(buf->buf + buf->len, s, len);
    buf->len += len;
    buf->buf[buf->len] = '\0';

    if (keep_null)
        buf->len++;

    return 0;
}

int sstore_push(string_store_t *store, char *s, size_t len)
{
    if (store->num_strings+1 > store->avail_displs)
    {
        store->avail_displs = store->num_strings+1;
        up2(store->avail_displs);
        store->displs = realloc(store->displs, store->avail_displs * sizeof(size_t));
    }

    size_t offset = store->buf.len;
    store->displs[store->num_strings++] = offset;

    string_push(&store->buf, s, len, 0);

    return 0;
}

const char *sstore_get_string(string_store_t store, size_t id)
{
    assert(id < store.num_strings);
    return store.buf.buf + store.displs[id];
}

size_t sstore_get_string_length(string_store_t store, size_t id)
{
    assert(id < store.num_strings);
    size_t endpos = (id == store.num_strings-1)? store.buf.len : store.displs[id+1];
    return endpos - store.displs[id];
}

size_t sstore_maxlen(string_store_t store)
{
    size_t maxlen = 0;

    for (size_t i = 0; i < store.num_strings; ++i)
    {
        size_t len = sstore_get_string_length(store, i);
        maxlen = len < maxlen? maxlen : len;
    }

    return maxlen;
}

size_t sstore_get_string_copy(string_store_t store, size_t id, char *s)
{
    size_t len = sstore_get_string_length(store, id);
    memcpy(s, store.buf.buf + store.displs[id], len);
    s[len] = '\0';
    return len;
}

char* sstore_get_string_dup(string_store_t store, size_t id)
{
    size_t len = sstore_get_string_length(store, id);
    char *s = malloc(len+1);
    sstore_get_string_copy(store, id, s);
    return s;
}

int sstore_mpi_bcast(string_store_t *store, int root, MPI_Comm comm)
{
    int nprocs, myrank;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &myrank);

    int info[2];
    string_t *buf = &store->buf;

    if (myrank == root)
    {
        /*
         * Release unused memory.
         */
        store->avail_displs = store->num_strings;
        store->displs = realloc(store->displs, store->avail_displs * sizeof(size_t));

        string_t *buf = &store->buf;
        buf->avail = buf->len+1;
        buf->buf = realloc(buf->buf, buf->avail);

        info[0] = (int)buf->len+1;
        info[1] = (int)store->num_strings;
    }

    MPI_Bcast(info, 2, MPI_INT, root, comm);

    if (myrank != root)
    {
        buf->avail = (size_t)info[0];
        buf->len = buf->avail-1;
        store->num_strings = store->avail_displs = (size_t)info[1];

        buf->buf = malloc(buf->avail);
        store->displs = malloc(store->num_strings * sizeof(size_t));
    }

    MPI_Bcast(buf->buf, info[0], MPI_CHAR, root, comm);
    MPI_Bcast(store->displs, info[1], MPI_SIZE_T, root, comm);

    return 0;
}

int sstore_mpi_scatter(const string_store_t *sendstore, string_store_t *recvstore, int root, MPI_Comm comm)
{
    int nprocs, myrank;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &myrank);

    // Every process needs to know how many strings it is receiving.
    // Every process needs to know how many chars it is receiving.
    // Every process needs to know the string_store displacements for the strings it receives.

    int globsize;
    size_t *displs_recvbuf;
    char *char_recvbuf;
    int *string_sendcounts;
    int *string_displs;
    int string_recvcount;
    int *char_sendcounts;
    int *char_displs;
    int char_recvcount;

    if (myrank == root)
    {
        globsize = sendstore->num_strings;
        string_sendcounts = malloc(nprocs * sizeof(int));
        string_displs = malloc(nprocs * sizeof(int));
        char_sendcounts = calloc(nprocs, sizeof(int));
        char_displs = malloc(nprocs * sizeof(int));
        *string_displs = 0;
        *char_displs = 0;

        for (int i = 0; i < nprocs-1; ++i)
        {
            string_sendcounts[i] = globsize / nprocs;
            string_displs[i+1] = string_displs[i] + string_sendcounts[i];
        }

        string_sendcounts[nprocs-1] = globsize - (nprocs-1)*(globsize / nprocs);

        for (int i = 0; i < nprocs; ++i)
        {
            for (int j = string_displs[i]; j < string_displs[i] + string_sendcounts[i]; ++j)
            {
                char_sendcounts[i] += sstore_get_string_length(*sendstore, j);
            }

            if (i != nprocs-1)
                char_displs[i+1] = char_displs[i] + char_sendcounts[i];
        }

    }

    MPI_Scatter(string_sendcounts, 1, MPI_INT, &string_recvcount, 1, MPI_INT, root, comm);
    MPI_Scatter(char_sendcounts, 1, MPI_INT, &char_recvcount, 1, MPI_INT, root, comm);

    displs_recvbuf = malloc(string_recvcount * sizeof(size_t));

    MPI_Scatterv(sendstore->displs, string_sendcounts, string_displs, MPI_SIZE_T, displs_recvbuf, string_recvcount, MPI_SIZE_T, root, comm);

    char_recvbuf = malloc(char_recvcount+1);

    MPI_Scatterv(sendstore->buf.buf, char_sendcounts, char_displs, MPI_CHAR, char_recvbuf, char_recvcount, MPI_CHAR, root, comm);

    char_recvbuf[char_recvcount] = 0;

    if (myrank == root)
    {
        free(string_sendcounts);
        free(string_displs);
        free(char_sendcounts);
        free(char_displs);
    }

    int displs_offset;
    MPI_Exscan(&char_recvcount, &displs_offset, 1, MPI_INT, MPI_SUM, comm);
    if (!myrank) displs_offset = 0;

    for (int i = 0; i < string_recvcount; ++i)
        displs_recvbuf[i] -= displs_offset;

    recvstore->buf = (string_t){char_recvbuf, char_recvcount, char_recvcount+1};
    recvstore->displs = displs_recvbuf;
    recvstore->avail_displs = recvstore->num_strings = string_recvcount;

    return 0;
}
