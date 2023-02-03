#include "elba_str.h"
#include "elba_error.h"
#include "size.h"
#include <stdarg.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

char const* const empty_str = "";

int elba_str_free(elba_str_t *s)
{
    if (!s) return ELBA_FAILURE;

    if (s->data) free(s->data);
    *s = (elba_str_t){0};

    return ELBA_SUCCESS;
}

int elba_str_clear(elba_str_t *s)
{
    if (!s) return ELBA_FAILURE;

    if (s->len > 0) s->data[s->len = 0] = (char)0;

    return ELBA_SUCCESS;
}

int elba_str_reserve(elba_str_t *s, size_t amt)
{
    char *data;
    size_t mem;

    if (!s) return ELBA_FAILURE;

    if (amt+1 < s->mem)
        return ELBA_SUCCESS;

    mem = next_pow2(amt+1);
    data = realloc(s->data, mem);

    if (!data) return ELBA_FAILURE;

    s->data = data;
    s->mem = mem;

    return ELBA_SUCCESS;
}

int elba_str_grow(elba_str_t *s, size_t amt)
{
    if (!s) return ELBA_FAILURE;

    return elba_str_reserve(s, amt + s->len);
}

int elba_str_lit(elba_str_t *s, char const *lit)
{
    size_t len;

    if (!s || !lit)
        return ELBA_FAILURE;

    elba_str_free(s);

    len = strlen(lit);

    if (elba_str_reserve(s, len) == ELBA_FAILURE)
        return ELBA_FAILURE;

    s->len = len;

    memcpy(s->data, lit, s->len);

    s->data[s->len] = '\0';

    return ELBA_SUCCESS;
}

int elba_str_move(elba_str_t *dest, elba_str_t *src)
{
    if (!dest || !src)
        return ELBA_FAILURE;

    elba_str_free(dest);

    *dest = *src;
    *src = (elba_str_t){0};

    return ELBA_SUCCESS;
}

int elba_str_copy(elba_str_t *dest, elba_str_t const *src)
{
    char *data;

    if (!dest || !src)
       return ELBA_FAILURE;

    data = realloc(dest->data, src->len+1);

    if (!data) return ELBA_FAILURE;

    memcpy(data, src->data, src->len);
    data[src->len] = 0;

    if (dest->data) free(dest->data);

    dest->data = data;
    dest->mem = src->len+1;
    dest->len = src->len;

    return ELBA_SUCCESS;
}

int elba_str_info(elba_str_t const s, FILE *f)
{
    fprintf(f, "{\"%s\", %lu, %lu}\n", s.data, s.len, s.mem);
    return ELBA_SUCCESS;
}

int elba_str_vprintf(elba_str_t *s, int truncate, char const *format, va_list args)
{
    char *buf, *dest;
    int size;
    size_t needed;

    size = vasprintf(&buf, format, args);
    needed = size + 1 + (truncate? 0 : s->len);

    if (needed > s->mem)
    {
        s->mem = next_pow2(needed);
        s->data = realloc(s->data, s->mem);

        if (!s->data) return ELBA_FAILURE;
    }

    dest = s->data + (truncate? 0 : s->len);

    memcpy(dest, buf, size);
    free(buf);

    s->len = truncate? size : s->len + size;
    s->data[s->len] = (char)0;

    return ELBA_SUCCESS;
}

int elba_str_ncat(elba_str_t *s, char const *lit, size_t n)
{
    if (!s || !lit || !n) return ELBA_FAILURE;

    if (elba_str_grow(s, n) != ELBA_SUCCESS)
        return ELBA_FAILURE;

    memcpy(s->data + s->len, lit, n);
    s->len += n;
    s->data[s->len] = (char)0;

    return ELBA_SUCCESS;
}

int elba_str_printf(elba_str_t *s, int truncate, char const *format, ...)
{
    int size;
    va_list args;
    va_start(args, format);
    size = elba_str_vprintf(s, truncate, format, args);
    va_end(args);
    return size;
}
