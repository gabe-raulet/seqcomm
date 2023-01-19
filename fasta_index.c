#include "fasta_index.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MPI_SIZE_T
#if SIZE_MAX == ULONG_MAX
#define MPI_SIZE_T MPI_UNSIGNED_LONG
#else
#error "size_t must be unsigned long"
#endif
#endif

/*
 * Get the next power-of-two greater than or equal to x.
 */

static inline size_t up_size_t(size_t x)
{
    if (x != 0)
        x--;

    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    x |= x>>32;

    return x+1;
}

int fasta_index_read(fasta_index_t *faidx, char const *fname, commgrid_t const *grid)
{
    int nprocs;       /* number of processes in comm                           */
    int myrank;       /* my process id in comm                                 */
    int *sendcounts;  /* MPI_Scatterv sendcounts for FAIDX records (root only) */
    int *displs;      /* MPI_Scatterv displs for FAIDX records (root only)     */
    int recvcount;    /* MPI_Scatterv recvcount for FAIDX records              */
    int ierr;         /* MPI error code                                        */

    size_t pos;         /* byte position within faidx file buffer (root only)         */
    size_t remain;      /* number of bytes remaining in faidx file buffer (root only) */
    size_t avail_recs;  /* number of records available in memory (root only)          */
    size_t linesize;    /* current line size (root only)                              */

    char *buf;   /* faidx file buffer (root only)         */
    char *ptr;   /* faidx file buffer pointer (root only) */
    char *next;  /* address of next '\n' char (root only) */

    MPI_File fh;
    MPI_Offset filesize;

    fasta_record_t *grecs, *myrecs, *rec;
    size_t num_recs;

    nprocs = grid->dims * grid->dims;
    myrank = grid->gridrank;

    if (myrank == 0)
    {
        /*
         * Root process slurps in the entire FAIDX file.
         */
        MPI_CHECK(MPI_File_open(MPI_COMM_SELF, fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh));

        MPI_File_get_size(fh, &filesize);

        buf = malloc(filesize);

        MPI_File_read(fh, buf, filesize, MPI_CHAR, MPI_STATUS_IGNORE);
        MPI_File_close(&fh);

        /*
         * Parse each FAIDX record from the file buffer.
         */
        ptr = buf;
        remain = filesize;
        pos = num_recs = avail_recs = 0;
        grecs = NULL;

        while (pos < filesize)
        {
            next = memchr(ptr, '\n', remain);

            if (next == NULL) /* TODO: this could be problematic if file does not have '\n' as its last character */
                break;

            *next = (char)0; /* TODO: otherwise sscanf might be a problem (not actually positive but there is no downside, test later */
            linesize = next - ptr;
            assert(linesize <= remain);

            if (num_recs + 1 > avail_recs)
            {
                avail_recs = up_size_t(num_recs + 1);
                avail_recs = avail_recs > 256? avail_recs : 256;
                grecs = realloc(grecs, avail_recs * sizeof(fasta_record_t));
            }

            rec = &grecs[num_recs++];
            sscanf(ptr, "%*s %zu %zu %zu %*zu", &rec->len, &rec->pos, &rec->width);

            /* when you get around to wanting read names, reference ../mpifa/mpifa.c */

            pos += linesize;
            ptr = &buf[++pos];
            remain = filesize - pos;
        }

        free(buf);
        grecs = realloc(grecs, num_recs * sizeof(fasta_index_t));

        sendcounts = malloc(nprocs * sizeof(int));
        displs = malloc(nprocs * sizeof(int));
        displs[0] = 0;

        /*
         * Processes with ids 0..nprocs-2 each get floor(num_recs/nprocs) FAIDX
         * records each. The process with id nprocs-1 gets all the remaining
         * sequences at the end.
         */
        for (int i = 0; i < nprocs-1; ++i)
        {
            sendcounts[i] = num_recs / nprocs;
            displs[i+1] = displs[i] + sendcounts[i];
        }

        sendcounts[nprocs-1] = num_recs - (nprocs-1)*(num_recs/nprocs);
    }

    /*
     * Root process tells each process how many FAIDX records it will be sent.
     */
    MPI_Scatter(sendcounts, 1, MPI_INT, &recvcount, 1, MPI_INT, 0, grid->grid_world);

    /*
     * Each process will receive its assigned portion of FAIDX records
     * at the location pointed to by myrecs.
     */
    myrecs = malloc(recvcount * sizeof(fasta_record_t));

    MPI_Datatype fasta_index_mpi_t;
    MPI_Type_contiguous(3, MPI_SIZE_T, &fasta_index_mpi_t);
    MPI_Type_commit(&fasta_index_mpi_t);

    MPI_Scatterv(grecs, sendcounts, displs, fasta_index_mpi_t, myrecs, recvcount, fasta_index_mpi_t, 0, grid->grid_world);
    MPI_Type_free(&fasta_index_mpi_t);

    if (myrank == 0)
    {
        free(grecs);
        free(sendcounts);
        free(displs);
    }

    faidx->records = myrecs;
    faidx->num_records = recvcount;
    faidx->grid = grid;

    return 0;
}
