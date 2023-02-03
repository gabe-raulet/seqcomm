#ifndef ELBA_COMM_H_
#define ELBA_COMM_H_

#include <mpi.h>
#include "elba_str_store.h"

/*
 * Broadcast the contents of a string storing container (elba_str_store_t)
 * from the given root process to every other process in the communicator
 * comm.
 */
int elba_comm_str_store_broadcast(elba_str_store_t *store, int root, MPI_Comm comm);

#endif
