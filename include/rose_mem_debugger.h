#ifndef RMDI_INCLUDE_H
#define RMDI_INCLUDE_H

/* Defines */
#ifndef RMD_WRAP_MALLOC_AND_FREE
#define RMD_WRAP_MALLOC_AND_FREE 1
#endif /* RMD_WRAP_MALLOC_AND_FREE */

#ifndef RMD_ENABLE_ASSERTS
#define RMD_ENABLE_ASSERTS 1
#endif /* RMD_ENABLE_ASSERTS */

#ifndef RMD_UNUSED
/* FIXME: This will only work on Linux! */
#define RMD_UNUSED __attribute__((unused))
#endif /* RMD_UNUSED */

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */

#ifndef RMDI_MAX_ALLOCS
#define RMDI_MAX_ALLOCS 4096
#endif /* RMDI_MAX_ALLOCS */

/* Unsigned types */
typedef unsigned char  rmd_u8;
typedef unsigned short rmd_u16;
typedef unsigned int   rmd_u32;
typedef unsigned long  rmd_size;

/* Signed types */
typedef signed char  rmd_s8;
typedef signed short rmd_s16;
typedef signed int   rmd_s32;

/* Floating point types */
typedef float  rmd_f32;
typedef double rmd_f64;

/* Other types */
typedef void   rmd_void;
typedef rmd_u8 rmd_bool;
enum { RMD_FALSE, RMD_TRUE };

/* Initialization flags */
enum {
        RMDF_PRINT_USAGE_AT_EXIT = (1u << 0u),
        RMDF_PRINT_HEAP_CALLS    = (1u << 1u),
        RMD_FLAG_MASK            = (RMDF_PRINT_USAGE_AT_EXIT |
                                    RMDF_PRINT_HEAP_CALLS)
};

typedef rmd_u8 rmd_flags_e;

/* Function macros */
#ifndef rmd_assert
#define rmd_assert(cond) \
        _rmd_assertf_internal(cond, #cond, __FILE__, __LINE__, NULL)
#endif /* rmd_assert */

#ifndef rmd_assertm
#define rmd_assertm(cond, msg) \
        _rmd_assertf_internal(cond, #cond, __FILE__, __LINE__, msg)
#endif /* rmd_assertm */

#ifndef rmd_assertf
#define rmd_assertf(cond, fmt, ...)                       \
        _rmd_assertf_internal(cond, #cond, __FILE__,      \
                              __LINE__, fmt, __VA_ARGS__)
#endif /* rmd_assertf */

/* Function prototypes */
extern rmd_void _rmdi_assertf_internal(const rmd_bool cond,
                                       const char *cond_str,
                                       const char *file, const int line,
                                       const char *fmt, ...);
extern struct rmdi_allocation *_rmdi_get_first_available_slot(void);

extern rmd_void rose_mem_debugger_init(const rmd_flags_e flags);
extern rmd_void rose_mem_debugger_terminate(void);

extern rmd_void *rmd_malloc(rmd_size sz);
extern rmd_void rmd_free(rmd_void *ptr);

extern rmd_void rmd_print_heap_usage(void);

#ifdef RMD_IMPLEMENTATION

/* Implementation includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Implementation types and structs */
struct rmdi_allocation {
        rmd_void *ptr;
        rmd_size  size;
        rmd_bool  in_use;
};

/* Implementation static variables */
static rmd_bool               rmdi_is_init                      = RMD_FALSE;
static rmd_flags_e            rmdi_flags                        = 0u;
static rmd_size               rmdi_bytes_allocated              = 0u;
static rmd_size               rmdi_num_allocations              = 0u;
static struct rmdi_allocation rmdi_allocations[RMDI_MAX_ALLOCS] = { 0u };

/* Implementation function definitions */
rmd_void _rmd_assertf_internal(const rmd_bool cond,
                               const char *cond_str,
                               const char *file, const int line,
                               const char *fmt, ...)
{
        if (!(RMD_ENABLE_ASSERTS))
                return;

        if (cond)
                return;

        if (!cond_str) {
                fprintf(stderr, "_rmd_assertf_internal(): no cond string\n");
                return;
        }

        if (!file) {
                fprintf(stderr, "_rmd_assertf_internal(): no file name\n");
                return;
        }

        if (line < 0) {
                fprintf(stderr, "_rmd_assertf_internal(): line num negative\n");
                return;
        }

        fprintf(stderr, "assertion `%s` failed [%s:%d]",
                cond_str, file, line);

        if (fmt) {
                va_list args;

                fprintf(stderr, " -> ");
                va_start(args, fmt);
                vfprintf(stderr, fmt, args);
                va_end(args);
        }

        fputc('\n', stderr);
        exit(-1);
}

rmd_void rose_mem_debugger_init(const rmd_flags_e flags)
{
        rmd_assertm(!rmdi_is_init,
                    "Rose Mem Debugger was already initialized!\n");

        if (flags & RMDF_PRINT_USAGE_AT_EXIT)
                atexit(&rmd_print_heap_usage);

        rmdi_bytes_allocated = 0u;
        rmdi_num_allocations = 0u;
        memset(rmdi_allocations, 0,
               RMDI_MAX_ALLOCS * sizeof(*rmdi_allocations));

        rmdi_flags   = flags;
        rmdi_is_init = RMD_TRUE;
}

rmd_void rose_mem_debugger_terminate(void)
{
        rmd_assertm(rmdi_is_init,
                    "Rose Mem Debugger was never initialized; can't free!\n");

        for (rmd_size i = 0u; i < RMDI_MAX_ALLOCS; ++i) {
                struct rmdi_allocation *a;

                a = rmdi_allocations + i;

                if (a->ptr || a->size || a->in_use) {
                        rmd_free(a->ptr);
                        a->ptr    = NULL;
                        a->size   = 0u;
                        a->in_use = RMD_FALSE;
                }
        }

        rmdi_bytes_allocated = 0u;
        rmdi_num_allocations = 0u;
        rmdi_flags           = 0u;
        rmdi_is_init         = RMD_FALSE;
}

struct rmdi_allocation *_rmdi_get_first_available_slot(void)
{
        for (rmd_size i = 0u; i < RMDI_MAX_ALLOCS; ++i) {
                struct rmdi_allocation *a;

                a = rmdi_allocations + i;
                if (a->in_use)
                        continue;

                return a;
        }

        return NULL;
}

rmd_void *rmd_malloc(rmd_size sz)
{
        struct rmdi_allocation *a;

        rmd_assertm(sz, "Trying to allocate with zero size!");

        rmdi_bytes_allocated += sz;

        a = _rmdi_get_first_available_slot();
        rmd_assertm(a != NULL, "Couldn't find allocation slot!");

        a->ptr = malloc(sz);

        rmd_assertm(a->ptr != NULL, "Allocation pointer failed to malloc()!");

        a->size = sz;

        ++rmdi_num_allocations;
        rmd_assertf(rmdi_num_allocations < RMDI_MAX_ALLOCS,
                    "Too many allocations (%lu)!\n", rmdi_num_allocations);

        a->in_use = RMD_TRUE;

        if (rmdi_flags & RMDF_PRINT_HEAP_CALLS) {
                printf("Allocated %lu bytes at slot %lu.\n",
                       a->size, (rmd_size)(a - rmdi_allocations));
        }

        return a->ptr;
}

rmd_void rmd_free(rmd_void *ptr)
{
        /* FIXME: Loop through allocations instead of integers */
        for (rmd_size i = 0u; i < RMDI_MAX_ALLOCS; ++i) {
                struct rmdi_allocation *a;

                a = rmdi_allocations + i;
                if (!a->in_use || ptr != a->ptr)
                        continue;

                free(a->ptr);

                --rmdi_num_allocations;
                rmdi_bytes_allocated -= a->size;

                if (rmdi_flags & RMDF_PRINT_HEAP_CALLS) {
                        printf("Freed %lu bytes from slot %lu\n",
                               a->size, (rmd_size)(a - rmdi_allocations));
                }

                a->ptr    = NULL;
                a->in_use = RMD_FALSE;
                a->size   = 0u;

                return;
        }

        rmd_assertf(0, "Failed to find pointer <%p> to free!", ptr);
}

rmd_void rmd_print_heap_usage(void)
{
        rmd_size bytes_counted  = 0u;
        rmd_size allocs_counted = 0u;

        for (rmd_size i = 0u; i < RMDI_MAX_ALLOCS; ++i) {
                struct rmdi_allocation *a;

                a = rmdi_allocations + i;
                if (a->in_use) {
                        ++allocs_counted;
                        rmd_assertm(a->size, "Suppose to have size!");
                        bytes_counted += a->size;
                } else {
                        rmd_assertm(!a->size, "Not supposed to have size!");
                        rmd_assertm(!a->ptr,  "Not supposed to have pointer!");
                }
        }

        rmd_assertf(bytes_counted == rmdi_bytes_allocated,
                    "Bytes allocated mismatch! (supposed to be %lu, is %lu)\n",
                    rmdi_bytes_allocated, bytes_counted);

        rmd_assertf(allocs_counted == rmdi_num_allocations,
                    "Num allocations mismatch! (supposed to be %lu, is %lu)\n",
                    rmdi_num_allocations, allocs_counted);

        printf("RMD Heap Usage: %lu bytes across %lu allocations.\n",
               bytes_counted, allocs_counted);
}

#endif /* RMD_IMPLEMENTATION */

/*
 * Handle malloc and free wrapping down here
 * outside implementation, but still inside header.
 */
#if RMD_WRAP_MALLOC_AND_FREE
#ifndef malloc
#define malloc(sz) rmd_malloc(sz)
#endif /* malloc */
#ifndef free
#define free(ptr) rmd_free(ptr)
#endif /* free */
#endif /* RMD_WRAP_MALLOC_AND_FREE */

#endif /* RMDI_INCLUDE_H */
