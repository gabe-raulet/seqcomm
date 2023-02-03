#include "elba_comm.h"
#include "elba_error.h"
#include "mpiutil.h"
#include "size.h"
#include <stdlib.h>
#include <assert.h>

int elba_comm_str_store_broadcast(elba_str_store_t *store, int root, MPI_Comm comm)
{
    elba_str_t *buf;
    int myrank, nprocs;
    size_t info[2];

    mpi_info(comm, &myrank, &nprocs);

    buf = &store->buf;

    if (myrank == root)
    {
        /*
         *  Release unused memory.
         */

        store->mem = store->size + 1;
        store->displs = realloc(store->displs, store->mem * sizeof(size_t));

        if (!store->displs) return ELBA_FAILURE;

        buf->mem = buf->len+1;
        buf->data = realloc(buf->data, buf->mem);

        if (!buf->data) return ELBA_FAILURE;

        info[0] = store->size;
        info[1] = buf->len;
    }

    MPI_Bcast(info, 2, MPI_SIZE_T, root, comm);

    if (myrank != root)
    {
        store->size = info[0];
        store->mem = store->size+1;
        buf->len = info[1];
        buf->mem = buf->len+1;

        buf->data = malloc(buf->mem);
        store->displs = malloc(store->mem * sizeof(size_t));

        if (!buf->data || !store->displs)
            return ELBA_FAILURE;
    }

    assert(is_safe_int(buf->mem));
    assert(is_safe_int(store->mem));

    MPI_Bcast(buf->data, (int)buf->mem, MPI_CHAR, root, comm);
    MPI_Bcast(store->displs, (int)store->mem, MPI_SIZE_T, root, comm);

    return ELBA_SUCCESS;
}
