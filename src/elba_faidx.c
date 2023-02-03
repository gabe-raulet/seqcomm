
#include "elba_faidx.h"
#include "elba_error.h"
#include "mpiutil.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int elba_fasta_index_read(elba_fasta_index_t *index, char const *fname, elba_str_store_t *names, commgrid_t const *grid)
{
    int nprocs;       /* number of processes in grid                            */
    int myrank;       /* my process id in grid                                  */
    int *sendcounts;  /* MPI_Scatterv sendcounts for  FAIDX records (root only) */
    int *displs;      /* MPI_Scatterv displs for FAIDX records (root only)      */
    int recvcount;    /* MPI_Scatterv recvcount for FAIDX records               */

    size_t pos;         /* byte position within faidx file buffer (root only)         */
    size_t remain;      /* number of bytes remaining in faidx file buffer (root only) */
    size_t avail_recs;  /* number of records available in memory (root only)          */
    size_t linesize;    /* current line size (root only)                              */

    char *buf;   /* faidx file buffer (root only)         */
    char *ptr;   /* faidx file buffer pointer (root only) */
    char *next;  /* address of next '\n' char (root only) */

    MPI_File fh;
    MPI_Offset filesize;

    elba_faidx_record_t *grecs, *myrecs, *rec;
    size_t num_recs;

    nprocs = commgrid_nprocs(*grid);
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

            if (!next) /* TODO: this could be problematic if file does not have '\n' as its last character */
                break;

            *next = (char)0;
            linesize = next - ptr;
            assert(linesize <= remain);

            if (num_recs + 1 > avail_recs)
            {
                avail_recs = next_pow2(num_recs + 1);
                avail_recs = avail_recs > 256? avail_recs : 256;
                grecs = realloc(grecs, avail_recs * sizeof(elba_faidx_record_t));
            }

            rec = &grecs[num_recs++];
            sscanf(ptr, "%*s %zu %zu %zu %*zu", &rec->len, &rec->pos, &rec->bases);

            if (names)
            {
                size_t namelen;

                for (namelen = 0; namelen < linesize; ++namelen)
                    if (isspace(ptr[namelen]))
                        break;

                elba_str_store_pushl(names, ptr, namelen);
            }

            pos += linesize;
            ptr = &buf[++pos];
            remain = filesize - pos;
        }

        free(buf);
        grecs = realloc(grecs, num_recs * sizeof(elba_faidx_record_t));

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

            if (sendcounts[i] == 0)
            {
                fprintf(stderr, "error: using too many processors\n");
                return ELBA_FAILURE;
            }
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
    myrecs = malloc(recvcount * sizeof(elba_faidx_record_t));

    MPI_Datatype elba_faidx_record_mpi_t;
    MPI_Type_contiguous(3, MPI_SIZE_T, &elba_faidx_record_mpi_t);
    MPI_Type_commit(&elba_faidx_record_mpi_t);
    MPI_Scatterv(grecs, sendcounts, displs, elba_faidx_record_mpi_t, myrecs, recvcount, elba_faidx_record_mpi_t, 0, grid->grid_world);
    MPI_Type_free(&elba_faidx_record_mpi_t);

    if (myrank == 0)
    {
        free(grecs);
        free(sendcounts);
        free(displs);
    }

    index->records = myrecs;
    index->num_records = recvcount;
    index->grid = grid;

    return ELBA_SUCCESS;
}

int elba_fasta_index_free(elba_fasta_index_t *index)
{
    if (!index) return ELBA_FAILURE;

    free(index->records);
    *index = (elba_fasta_index_t){0};
    return ELBA_SUCCESS;
}

int elba_fasta_index_log(elba_fasta_index_t const index, char const *fname, FILE *f)
{
    if (!fname || !*fname || !f)
        return ELBA_FAILURE;

    char tag[256];
    commgrid_tag(*index.grid, tag);

    fprintf(f, "[%s] read %lu FAIDX records from '%s'\n", tag, index.num_records, fname);
    fflush(f);

    MPI_Barrier(index.grid->grid_world);

    return ELBA_SUCCESS;
}
