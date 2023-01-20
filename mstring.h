#ifndef MSTRING_H_
#define MSTRING_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <mpi.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct s_string
{
    char *buf;
    size_t len;
    size_t avail;
} string_t;

typedef struct s_string_store
{
    string_t buf;
    size_t num_strings;
    size_t avail_displs;
    size_t *displs;
} string_store_t;

#define STRING_INIT (string_t){0}
#define STRING_NEW ((string_t*)calloc(1, sizeof(string_t)))

#define string_destroy(s) do { \
    free((s).buf); \
    memset(&(s), 0, sizeof(string_t)); \
} while (0);

#define string_free(s) do { \
    string_destroy((*s)); \
    free((s)); \
} while (0)

int string_printf(string_t *buf, int truncate, const char *format, ...);
int string_push(string_t *buf, char *s, size_t len, int keep_null);

#define string_catf(buf, fmt, ...)   string_printf((buf), 0, (fmt), ##__VA_ARGS__)
#define string_truncf(buf, fmt, ...) string_printf((buf), 1, (fmt), ##__VA_ARGS__)

#define STRING_STORE_INIT (string_store_t){0}

#define string_store_destroy(ss) do { \
    free((ss).displs); \
    string_destroy((ss).buf); \
    memset(&(ss), 0, sizeof(string_store_t)); \
} while (0);

int sstore_push(string_store_t *store, char *s, size_t len);
const char *sstore_get_string(string_store_t store, size_t id);
size_t sstore_maxlen(string_store_t store);
size_t sstore_get_string_length(string_store_t store, size_t id);
size_t sstore_get_string_copy(string_store_t store, size_t id, char *s);
char* sstore_get_string_dup(string_store_t store, size_t id);
int sstore_mpi_scatter(const string_store_t *sendstore, string_store_t *recvstore, int root, MPI_Comm comm);
int sstore_mpi_bcast(string_store_t *store, int root, MPI_Comm comm);

#define sstore_push_const(store, s) sstore_push((store), (s), strlen((s)))

#endif
