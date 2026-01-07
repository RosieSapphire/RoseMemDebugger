#ifndef RMDI_INCLUDE_H
#define RMDI_INCLUDE_H

/*
 * TODO at some point:
 * I might wanna find a way for the user
 * to define a single macro to completely
 * disable the compilation of this translation
 * unit until they actually want it.
 *
 * Something like RMD_DISABLE_ALL_FUNCTIONALITY
 * or whatever should do the trick, but I'm not *super*
 * worried about it right now.
 */

/*
 * There are some configuration macros you can use that will
 * allow you to enable or disable certain things about the API.
 *
 * Some examples are:
 * - RMD_ENABLE_WRAPPING
 * - RMD_DISABLE_ASSERTS
 * - RMD_MAX_ALLOCS
 * - RMD_STRICT_FREE
 * - RMD_FREE_ALL_SLOTS_ON_TERMINATE
 *
 * Although, I haven't fully planned this out yet, so feel
 * free to look through the code for right now. lmao
 */

/* Includes */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Defines */
#ifndef RMD_MAX_ALLOCS
#define RMD_MAX_ALLOCS 4096
#endif /* RMD_MAX_ALLOCS */

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
        RMDF_NONE                = (0u),
        RMDF_PRINT_HEAP_CALLS    = (1u << 0u),
        RMD_FLAG_MASK            = (RMDF_PRINT_HEAP_CALLS)
} rmd_flags_e;

/* Function macros */
#ifndef rmd_assert
#define rmd_assert(cond) \
        _rmdi_assertf_internal(cond, #cond, __FILE__, __LINE__, NULL)
#endif /* rmd_assert */

/* `fmt` or `msg` must be the first variadic parameter here! */
#ifndef rmd_assertf
#define rmd_assertf(cond, ...) \
        _rmdi_assertf_internal(cond, #cond, __FILE__, __LINE__, __VA_ARGS__)
#endif /* rmd_assertf */

#ifndef rmd_malloc
#define rmd_malloc(x) _rmdi_malloc_internal(x, __FILE__, __LINE__)
#endif /* rmd_malloc */

#ifndef rmd_free
#define rmd_free(x) _rmdi_free_internal(x, __FILE__, __LINE__)
#endif /* rmd_free */

/* Function prototypes */
extern rmd_void rose_mem_debugger_init(const rmd_flags_e flags);
extern rmd_void rose_mem_debugger_terminate(void);

extern rmd_void *_rmdi_malloc_internal(rmd_size sz, const char *file_name,
                                       const int line_num);
extern rmd_void _rmdi_free_internal(rmd_void *ptr, const char *file_name,
                                    const int line_num);

extern rmd_void rmd_print_heap_usage(void);

/*
 * NOTE:
 * Okay, the only reason that I have this here is
 * because of the way my syntax highlighter or LSP
 * or whatever works; it'll only un-grey the code
 * below if the macro is enabled.
 *
 * Unfortunately, I have a bad habit of leaving
 * this on when committing, so I am just making a note
 * of this stupid bullshit right now. lmfao
 */
#if 0
#define RMD_IMPLEMENTATION
#endif

#ifdef RMD_IMPLEMENTATION
#ifndef RMD_IMPLEMENTATION_GAURD
#define RMD_IMPLEMENTATION_GAURD
#else /* RMD_IMPLEMENTATION_GAURD */
#error "RMD_IMPLEMENTATION is defined more than once!"
#endif /* RMD_IMPLEMENTATION_GAURD */

/* Implementation types and structs */
struct rmdi_block {
        rmd_void   *ptr;
        rmd_size    req_size;
        rmd_bool    in_use;
        const char *file; /* NOTE: This must be a static string pointer! */
        int         line;
};

/* Implementation static variables */
static rmd_bool          rmdi_is_init                 = RMD_FALSE;
static rmd_flags_e       rmdi_flags                   = 0u;
static rmd_size          rmdi_bytes_allocated         = 0u;
static rmd_size          rmdi_num_allocations         = 0u;
static struct rmdi_block rmdi_blocks[RMD_MAX_ALLOCS]  = { 0u };

/* Implementation function definitions */
static rmd_void _rmdi_assertf_internal(const rmd_bool cond,
                                       const char *cond_str,
                                       const char *file, const int line,
                                       const char *fmt, ...)
{
#ifdef RMD_DISABLE_ASSERTS
        (void)cond;
        (void)cond_str;
        (void)file;
        (void)line;
        (void)fmt;
        return;
#else /* RMD_DISABLE_ASSERTS */
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
        exit(EXIT_FAILURE);
#endif /* RMD_DISABLE_ASSERTS */
}

rmd_void rose_mem_debugger_init(const rmd_flags_e flags)
{
        rmd_assertf(!rmdi_is_init,
                    "Rose Mem Debugger was already initialized!");

        rmdi_bytes_allocated = 0u;
        rmdi_num_allocations = 0u;
        memset(rmdi_blocks, 0, RMD_MAX_ALLOCS * sizeof(*rmdi_blocks));

        rmdi_flags   = flags;
        rmdi_is_init = RMD_TRUE;
}

rmd_void rose_mem_debugger_terminate(void)
{
        rmd_assertf(rmdi_is_init,
                    "Rose Mem Debugger was never initialized; can't free!");

        /*
         * This function *does* free all the stuff that was
         * current allocated, but before doing that, it's usually
         * a good idea to make sure that we see how much memory
         * *was* in use before it was flushed out so we can
         * go back and fix that!
         */
        rmd_print_heap_usage();

#ifdef RMD_FREE_ALL_SLOTS_ON_TERMINATE
        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a) {
                if (!a->ptr)
                        continue;

                rmd_assertf(a->req_size, "Pointer <%p> is allocated, "
                                         "and should have size, "
                                         "but doesn't.", a->ptr);
                rmd_assertf(a->in_use, "Pointer <%p> is allocated "
                                       "and has size %zu, but is not "
                                       "marked as `in_use`.\n", a->ptr);

                rmd_free(a->ptr);

                a->ptr      = NULL;
                a->req_size = 0u;
                a->in_use   = RMD_FALSE;
                a->file     = NULL;
                a->line     = -1;
        }
#endif /* RMD_FREE_ALL_SLOTS_ON_TERMINATE */

        rmdi_bytes_allocated = 0u;
        rmdi_num_allocations = 0u;
        rmdi_flags           = 0u;
        rmdi_is_init         = RMD_FALSE;
}

static struct rmdi_block *_rmdi_get_first_available_slot(void)
{
        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a)
                if (!a->in_use)
                        return a;

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

rmd_void *_rmdi_malloc_internal(rmd_size sz, const char *file_name,
                                const int line_num)
{
        struct rmdi_block *a;
        
        rmd_assertf(rmdi_is_init, "Trying to call `rmd_malloc()` before"
                                  "`rose_mem_debugger_init() was called.`");

        rmd_assertf(sz, "Trying to allocate with zero size!");

        a = _rmdi_get_first_available_slot();
        rmd_assertf(a != NULL, "Couldn't find allocation slot!");
        rmd_assertf(!a->in_use, "Trying to allocate to a slot "
                                "that is already marked as in use");

        a->ptr = _rmdi_system_malloc(sz);
        rmd_assertf(a->ptr != NULL, "Allocation pointer failed to malloc()!");

        rmdi_bytes_allocated += sz;
        a->req_size           = sz;

        ++rmdi_num_allocations;
        rmd_assertf(rmdi_num_allocations < RMD_MAX_ALLOCS,
                    "Too many allocations (%zu)!\n", rmdi_num_allocations);

        if (rmdi_flags & RMDF_PRINT_HEAP_CALLS) {
                printf("malloc(%zu B) [s%zu] -> [%s:%d].\n",
                       a->req_size, (rmd_size)(a - rmdi_blocks),
                       file_name, line_num);
        }

        a->in_use = RMD_TRUE;
        a->file   = file_name;
        a->line   = line_num;

        return a->ptr;
}

/*
 * This function returns `NULL` upon success.
 * I've noticed this function looks almost identical
 * to `_rmdi_get_first_available_slot()`, so I wonder
 * if there's a way to integrate them. Probably not a big concern rn tho. lol
 *
 * ALSO, NOTE:
 * This function does not account for:
 * - freeing a random pointer
 * - freeing a pointer allocated before init (which shouldn't happen anyway)
 * - freeing memory from another allocator (obviously, I think)
 *
 * So, uh... user beware...?
 */
static struct rmdi_block *_rmdi_is_double_free(rmd_void *ptr_trying_free)
{
        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a)
                if (!a->in_use && a->ptr == ptr_trying_free)
                        return a;

        return NULL;
}

rmd_void _rmdi_free_internal(rmd_void *ptr, const char *file_name,
                             const int line_num)
{
        rmd_assertf(rmdi_is_init, "Trying to call `rmd_free()` before"
                                  "`rose_mem_debugger_init() was called.`");

#ifdef RMD_STRICT_FREE
        rmd_assertf(ptr != NULL, "Trying to free a NULL pointer <%p>!", ptr);
#else /* RMD_STRICT_FREE */
        if (!ptr)
                return;
#endif /* RMD_STRICT_FREE */

        /* Before doing anything, make sure it's not a double-free! */
#ifdef RMD_STRICT_FREE
                rmd_assertf(!_rmdi_is_double_free(ptr),
                            "Attempting to double-free pointer <%p> "
                            "at [%s:%d]", ptr, file_name, line_num);
#else /* RMD_STRICT_FREE */
        if (_rmdi_is_double_free(ptr)) {
                fprintf(stderr, "Attempting to double-free pointer <%p> "
                                "at [%s:%d]", ptr, file_name, line_num);
                return;
        }
#endif /* RMD_STRICT_FREE */

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
        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a) {
                if (!a->in_use || ptr != a->ptr)
                        continue;

                _rmdi_system_free(a->ptr);

                --rmdi_num_allocations;
                rmdi_bytes_allocated -= a->req_size;

                if (rmdi_flags & RMDF_PRINT_HEAP_CALLS) {
                        printf("free(%zu B) [s%zu] -> [%s:%d].\n",
                               a->req_size, (rmd_size)(a - rmdi_blocks),
                               file_name, line_num);
                }

                a->in_use = RMD_FALSE;

                return;
        }

#ifdef RMD_STRICT_FREE
        rmd_assertf(0, "Failed to find pointer <%p> to free!", ptr);
#endif /* RMD_STRICT_FREE */
}

rmd_void rmd_print_heap_usage(void)
{
        rmd_size bytes_counted  = 0u;
        rmd_size allocs_counted = 0u;

        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a) {
                if (!a->in_use)
                        continue;

                ++allocs_counted;
                rmd_assertf(a->req_size, "Suppose to have size!");
                bytes_counted += a->req_size;
        }

        rmd_assertf(bytes_counted == rmdi_bytes_allocated,
                    "Bytes allocated mismatch! (supposed to be %zu, is %zu)\n",
                    rmdi_bytes_allocated, bytes_counted);

        rmd_assertf(allocs_counted == rmdi_num_allocations,
                    "Num allocations mismatch! (supposed to be %zu, is %zu)\n",
                    rmdi_num_allocations, allocs_counted);

        printf("RMD Heap Usage: %zu bytes across %zu allocations.\n",
               bytes_counted, allocs_counted);

        /*
         * If no memory was left over, then we're done here.
         * Otherwise, we print out all the blocks that were
         * still allocated on the heap!
         */
        if (!bytes_counted && !allocs_counted)
                return;

        for (struct rmdi_block *a = rmdi_blocks;
             (rmd_size)(a - rmdi_blocks) < RMD_MAX_ALLOCS; ++a)
                if (a->in_use)
                        printf("LEAK: [s%zu] -> %zu B -> <%p> -> [%s:%d]\n",
                               (rmd_size)(a - rmdi_blocks), a->req_size,
                               a->ptr, a->file, a->line);
}
#endif /* RMD_IMPLEMENTATION */

/*
 * Handle malloc and free wrapping down here
 * outside implementation, but still inside header.
 */
#ifdef RMD_ENABLE_WRAPPING
/*
 * FIXME:
 * Eventually find a better way to do this. This fucks
 * with 3rd party headers and other parts of the program.
 */
#define malloc(sz) rmd_malloc(sz)
#define free(ptr) rmd_free(ptr)
#endif /* RMD_ENABLE_WRAPPING */

#endif /* RMDI_INCLUDE_H */
