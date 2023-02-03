
#include "elba_str_store.h"
#include "elba_error.h"

int elba_str_store_pushl(elba_str_store_t *store, char const *s, size_t len)
{
    if (!store || !s || !len)
        return ELBA_FAILURE;

    if (store->size + 2 > store->mem)
    {
        store->mem += 4096;
        store->displs = realloc(store->displs, store->mem * sizeof(size_t));

        if (!store->displs) return ELBA_FAILURE;
    }

    if (elba_str_ncat(&store->buf, s, len) != ELBA_SUCCESS)
        return ELBA_FAILURE;

    if (store->size == 0) store->displs[0] = 0;

    store->displs[++store->size] = elba_str_len(store->buf);

    return ELBA_SUCCESS;
}

int elba_str_store_get_len(elba_str_store_t const store, size_t id, size_t *len)
{
    if (id >= store.size || !len) return ELBA_FAILURE;

    *len = store.displs[id+1] - store.displs[id];

    return ELBA_SUCCESS;
}

int elba_str_store_get_maxlen(elba_str_store_t const store, size_t *maxlen)
{
    if (!maxlen) return ELBA_FAILURE;

    size_t len, max = 0;

    for (size_t i = 0; i < store.size; ++i)
    {
        len = store.displs[i+1] - store.displs[i];
        max = len > max? len : max;
    }

    *maxlen = max;
    return ELBA_SUCCESS;
}

static inline int elba_str_store_get_helper(elba_str_store_t const *store, size_t id, char **s, int alloc)
{
    if (id >= store->size || !s) return ELBA_FAILURE;

    size_t len = store->displs[id+1] - store->displs[id];

    if (alloc)
    {
        *s = malloc(len+1);
        if (!*s) return ELBA_FAILURE;
    }

    memcpy(*s, store->buf.data + store->displs[id], len);
    (*s)[len] = (char)0;

    return ELBA_SUCCESS;
}

int elba_str_store_get_strcpy(elba_str_store_t const store, size_t id, char *s)
{
    return elba_str_store_get_helper(&store, id, &s, 0);
}

int elba_str_store_get_strdup(elba_str_store_t const store, size_t id, char **s)
{
    return elba_str_store_get_helper(&store, id, s, 1);
}

int elba_str_store_free(elba_str_store_t *store)
{
    if (!store) return ELBA_FAILURE;

    if (elba_str_free(&store->buf) != ELBA_SUCCESS)
        return ELBA_FAILURE;

    free(store->displs);

    *store = ELBA_STR_STORE_INIT;

    return ELBA_SUCCESS;
}
