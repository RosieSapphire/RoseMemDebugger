#ifndef RMDI_INCLUDE_H
#define RMDI_INCLUDE_H

/*
 * There are some configuration macros you can use that will
 * allow you to enable or disable certain things about the API.
 *
 * Some examples are:
 * - RMD_WRAP_MALLOC_AND_FREE
 * - RMD_NO_INCLUDE_STDDEF
 * - RMD_ENABLE_ASSERTS
 * - RMD_MAX_ALLOCS
 *
 * Although, I haven't fully planned this out yet, so feel
 * free to look through the code for right now. lmao
 */

/* Defines */
#ifndef RMD_WRAP_MALLOC_AND_FREE
#define RMD_WRAP_MALLOC_AND_FREE 1
#endif /* RMD_WRAP_MALLOC_AND_FREE */

#ifndef RMD_NO_INCLUDE_STDDEF
#include <stddef.h>
#endif /* RMD_NO_INCLUDE_STDDEF */


#ifndef RMD_ENABLE_ASSERTS
#define RMD_ENABLE_ASSERTS 1
#endif /* RMD_ENABLE_ASSERTS */

#ifndef RMD_UNUSED
/* FIXME: This will only work on Linux! */
#define RMD_UNUSED __attribute__((unused))
#endif /* RMD_UNUSED */

#ifndef RMDI_MAX_ALLOCS
#define RMDI_MAX_ALLOCS 4096
#endif /* RMDI_MAX_ALLOCS */

/* Unsigned types */
typedef unsigned char  rmd_u8;
typedef unsigned short rmd_u16;
typedef unsigned int   rmd_u32;
typedef size_t         rmd_size;

/* Signed types */
typedef signed char  rmd_s8;
typedef signed short rmd_s16;
typedef signed int   rmd_s32;

/* Floating point types */
typedef float  rmd_f32;
typedef double rmd_f64;

/* Other types */
typedef void rmd_void;
typedef enum {
        RMD_FALSE = 0u,
        RMD_TRUE = 1u
} rmd_bool;

/* Initialization flags */
typedef enum {
        RMDF_PRINT_USAGE_AT_EXIT = (1u << 0u),
        RMDF_PRINT_HEAP_CALLS    = (1u << 1u),
        RMD_FLAG_MASK            = (RMDF_PRINT_USAGE_AT_EXIT |
                                    RMDF_PRINT_HEAP_CALLS)
} rmd_flags_e;

/* Function macros */
#ifndef rmd_assert
#define rmd_assert(cond) \
        _rmdi_assertf_internal(cond, #cond, __FILE__, __LINE__, NULL)
#endif /* rmd_assert */

#ifndef rmd_assertm
#define rmd_assertm(cond, msg) \
        _rmdi_assertf_internal(cond, #cond, __FILE__, __LINE__, msg)
#endif /* rmd_assertm */

#ifndef rmd_assertf
#define rmd_assertf(cond, fmt, ...)                       \
        _rmdi_assertf_internal(cond, #cond, __FILE__,      \
                              __LINE__, fmt, __VA_ARGS__)
#endif /* rmd_assertf */

/* Function prototypes */
extern rmd_void rose_mem_debugger_init(const rmd_flags_e flags);
extern rmd_void rose_mem_debugger_terminate(void);

extern rmd_void *rmd_malloc(rmd_size sz);
extern rmd_void rmd_free(rmd_void *ptr);

extern rmd_void rmd_print_heap_usage(void);

/* FIXME: TMP */
#if 1
#define RMD_IMPLEMENTATION
#endif

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
static rmd_void _rmdi_assertf_internal(const rmd_bool cond,
                                       const char *cond_str,
                                       const char *file, const int line,
                                       const char *fmt, ...)
{
        if (!(RMD_ENABLE_ASSERTS))
                return;

        if (cond)
                return;

        if (!cond_str) {
                fprintf(stderr, "_rmdi_assertf_internal(): no cond string\n");
                return;
        }

        if (!file) {
                fprintf(stderr, "_rmdi_assertf_internal(): no file name\n");
                return;
        }

        if (line < 0) {
                fprintf(stderr, "_rmdi_assertf_internal(): line num negative\n");
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

                if (a->ptr) {
                        rmd_assertf(a->size, "Pointer <%p> is allocated, "
                                             "and should have size, "
                                             "but doesn't.", a->ptr);
                        rmd_assertf(a->in_use, "Pointer <%p> is allocated "
                                               "and has size %lu, but is not "
                                               "marked as `in_use`.\n", a->ptr);
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

static struct rmdi_allocation *_rmdi_get_first_available_slot(void)
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

static __inline rmd_void *_rmdi_system_malloc(rmd_size sz)
{
        return malloc(sz);
}

static __inline rmd_void _rmdi_system_free(rmd_void *ptr)
{
        free(ptr);
}

rmd_void *rmd_malloc(rmd_size sz)
{
        struct rmdi_allocation *a;

        rmd_assertm(sz, "Trying to allocate with zero size!");

        rmdi_bytes_allocated += sz;

        a = _rmdi_get_first_available_slot();
        rmd_assertm(a != NULL, "Couldn't find allocation slot!");

        a->ptr = _rmdi_system_malloc(sz);

        rmd_assertm(a->ptr != NULL, "Allocation pointer failed to malloc()!");

        a->size = sz;

        /*
         * FIXME: This should print out a whole page of
         * debug shit including all the currently allocated
         * blocks, where (and possibly when) they were allocated,
         * and probably some other shit. This is more for later though
         */
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
        rmd_assertf(ptr != NULL, "Trying to free a NULL pointer <%p>!", ptr);

        /*
         * FIXME:
         * So, this currently just does a linear search through
         * all the allocated blocks, which is fine, but at some point,
         * if I wanna make this run better, it would be a good idea
         * to optimize this at some point with a hash table or some shit. :3
         *
         * This could be a subject of premature optimization, so
         * don't worry about it too much for now. :D
         */
        for (rmd_size i = 0u; i < RMDI_MAX_ALLOCS; ++i) {
                struct rmdi_allocation *a;

                a = rmdi_allocations + i;
                if (!a->in_use || ptr != a->ptr)
                        continue;

                _rmdi_system_free(a->ptr);

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
