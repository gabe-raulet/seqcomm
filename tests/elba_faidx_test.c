#include "elba_faidx.h"
#include "elba_error.h"
#include "elba_str.h"
#include "elba_str_store.h"
#include "elba_comm.h"
#include "commgrid.h"
#include "mpiutil.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int myrank = mpi_get_myrank(MPI_COMM_WORLD);
    int nprocs = mpi_get_nprocs(MPI_COMM_WORLD);

    commgrid_t grid;

    ELBA_CHECK(commgrid_init(&grid, MPI_COMM_WORLD));
    commgrid_log(grid, stderr);

    elba_str_t fname = ELBA_STR_INIT;
    elba_str_lit(&fname, argv[1]);
    elba_str_cat(&fname, ".fai");
    /* elba_str_info(fname, stderr); */

    elba_fasta_index_t index;
    elba_str_store_t names = ELBA_STR_STORE_INIT;

    ELBA_CHECK(elba_fasta_index_read(&index, elba_str_str(fname), &names, &grid));
    ELBA_CHECK(elba_comm_str_store_broadcast(&names, 0, grid.grid_world));

    size_t max_namelen;
    ELBA_CHECK(elba_str_store_get_maxlen(names, &max_namelen));

    char *namebuf = malloc(max_namelen+1);

    size_t num_records = index.num_records;

    for (size_t i = 0; i < num_records; ++i)
    {
        elba_faidx_record_t record = index.records[i];
        ELBA_CHECK(elba_str_store_get_strcpy(names, i, namebuf));

        /* printf("[%d] %lu,%lu,%lu\n", myrank, record.len, record.pos, record.bases); */
        printf("[%d] %s,%lu,%lu,%lu\n", myrank, namebuf, record.len, record.pos, record.bases);
    }

    free(namebuf);

    ELBA_CHECK(elba_fasta_index_free(&index));

    MPI_Finalize();
    return 0;
}
