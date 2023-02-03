#ifndef ELBA_STR_H_
#define ELBA_STR_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

typedef struct { char *data; size_t len, mem; } elba_str_t;

static inline char* elba_str_data(elba_str_t const s) { return s.data; }
static inline size_t elba_str_len(elba_str_t const s) { return s.len; }
static inline size_t elba_str_mem(elba_str_t const s) { return s.mem; }

static inline char const* const elba_str_str(elba_str_t const s)
{
    extern char const* const empty_str;
    return (!s.len || !s.data)? empty_str : (char const* const) s.data;
}

int elba_str_free(elba_str_t *s);
int elba_str_clear(elba_str_t *s);
int elba_str_reserve(elba_str_t *s, size_t amt);
int elba_str_grow(elba_str_t *s, size_t amt);
int elba_str_lit(elba_str_t *s, char const *lit);
int elba_str_move(elba_str_t *dest, elba_str_t *src);
int elba_str_copy(elba_str_t *dest, elba_str_t const *src);
int elba_str_printf(elba_str_t *s, int truncate, char const *format, ...);
int elba_str_ncat(elba_str_t *s, char const *lit, size_t n);
int elba_str_info(elba_str_t const s, FILE *f);

#define ELBA_STR_INIT (elba_str_t){0}
#define ELBA_STR_INIT_MEM ((elba_str_t){calloc(16,1), 0, 16})
#define ELBA_STR_ARRAY_INIT(n) (elba_str_t*)calloc((n), sizeof(elba_str_t))
#define ELBA_STR_LIT(lit) (elba_str_t){strdup((lit)), strlen((lit)), strlen((lit))+1}
#define ELBA_STR_DUP(elba_str) ELBA_STR_LIT((elba_str).data)

#define elba_str_cat(s, lit) elba_str_ncat((s), (lit), strlen((lit)))
#define elba_str_catf(s, fmt, ...)   elba_str_printf((s), 0, (fmt), ##__VA_ARGS__)
#define elba_str_truncf(s, fmt, ...) elba_str_printf((s), 1, (fmt), ##__VA_ARGS__)

#endif
