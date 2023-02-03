#include "elba_str_store.h"
#include "elba_error.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    elba_str_store_t store = ELBA_STR_STORE_INIT;

    elba_str_store_push(&store, "Packt Like Sardines in a Crushd Tin Box");
    elba_str_store_push(&store, "Pyramid Song");
    elba_str_store_push(&store, "Pulk/Pull Revolving Doors");
    elba_str_store_push(&store, "You and Whose Army?");
    elba_str_store_push(&store, "I Might Be Wrong");
    elba_str_store_push(&store, "Knives Out");
    elba_str_store_push(&store, "Morning Bell/Amnesiac");
    elba_str_store_push(&store, "Dollars and Cents");
    elba_str_store_push(&store, "Hunting Bears");
    elba_str_store_push(&store, "Like Spinning Plates");
    elba_str_store_push(&store, "Life in a Glasshouse");

    size_t maxlen;
    ELBA_CHECK(elba_str_store_get_maxlen(store, &maxlen));

    char *buf = malloc(maxlen+1);

    for (size_t i = 0; i < store.size; ++i)
    {
        ELBA_CHECK(elba_str_store_get_strcpy(store, i, buf));
        printf("%s\n", buf);
    }

    free(buf);
    fputc('\n', stdout);

    for (size_t i = 0; i < store.size; ++i)
    {
        char *s;
        size_t len;
        ELBA_CHECK(elba_str_store_get_strdup(store, i, &s));
        ELBA_CHECK(elba_str_store_get_len(store, i, &len));
        printf("%lu: %s\n", len, s);
        free(s);
    }

    elba_str_store_free(&store);

    MPI_Finalize();
    return 0;
}
