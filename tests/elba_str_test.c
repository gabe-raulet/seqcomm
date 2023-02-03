#include "elba_str.h"
#include "elba_error.h"
#include <assert.h>

/* #ifdef ELBA_CHECK */
/* #undef ELBA_CHECK */
/* #define ELBA_CHECK */
/* #endif */


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    elba_str_t s = ELBA_STR_INIT;
    elba_str_info(s, stderr);

    ELBA_CHECK(elba_str_lit(&s, "ZZZ"));
    elba_str_info(s, stderr);

    assert(elba_str_len(s) == 3);
    assert(!strcmp(elba_str_str(s), "ZZZ"));
    assert(elba_str_mem(s) >= 3+1);

    ELBA_CHECK(elba_str_free(&s));
    elba_str_info(s, stderr);

    ELBA_CHECK(elba_str_catf(&s, "%c,%.4f,%s,%d", 's', 3.141592, "hello_world!", 421));
    elba_str_info(s, stderr);

    assert(!strcmp(elba_str_str(s), "s,3.1416,hello_world!,421"));

    ELBA_CHECK(elba_str_catf(&s, ",%d,%s,%.2f", 70, "goodbye_world...",5.55));
    elba_str_info(s, stderr);

    assert(!strcmp(elba_str_str(s), "s,3.1416,hello_world!,421,70,goodbye_world...,5.55"));

    ELBA_CHECK(elba_str_truncf(&s, "RESET DEFCON %d", 4));
    assert(!strcmp(elba_str_str(s), "RESET DEFCON 4"));
    elba_str_info(s, stderr);

    ELBA_CHECK(elba_str_clear(&s));
    assert(!strcmp(elba_str_str(s), ""));
    assert(elba_str_len(s) == 0);
    elba_str_info(s, stderr);

    ELBA_CHECK(elba_str_free(&s));
    elba_str_info(s, stderr);

    assert(elba_str_data(s) == NULL);
    assert(elba_str_len(s) == 0);
    assert(elba_str_mem(s) == 0);
    assert(!strcmp(elba_str_str(s), ""));

    ELBA_CHECK(elba_str_free(&s));
    elba_str_info(s, stderr);


    MPI_Finalize();
    return 0;
}
