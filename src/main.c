#include <stdlib.h>
#include <time.h>

#define RMD_STRICT_FREE
#define RMD_IMPLEMENTATION
#include "rose_mem_debugger.h"

static rmd_void inds_randomize(rmd_u8 *lst, const rmd_u8 cnt)
{
        /* Generate linear deck */
        for (rmd_u8 i = 0u; i < cnt; ++i)
                lst[i] = i;

        /* Shuffle it */
        for (rmd_u8 i = (cnt - 1u); i > 0u; --i) {
                rmd_u8 *ba  = NULL;
                rmd_u8 *bb  = NULL;
                rmd_u8  tmp = 0u;
                rmd_u8  j   = 0u;

                j   = (rmd_u8)(rand() % (i + 1u));
                ba  = lst + i;
                bb  = lst + j;
                tmp = *bb;
                *bb = *ba;
                *ba = tmp;
        }
}

int main(void)
{
        rmd_u8   alloc_cnt = 13u;
        rmd_u8 **allocs    = NULL;
        rmd_u8   inds[13u] = { 0u };

        srand(time(NULL));
        // rose_mem_debugger_init(RMDF_PRINT_HEAP_CALLS);
        rose_mem_debugger_init(RMDF_NONE);

        allocs = malloc(sizeof(*allocs) * alloc_cnt);

        inds_randomize(inds, alloc_cnt);
        for (rmd_u8 i = 0u; i < alloc_cnt; ++i)
                allocs[inds[i]] = malloc(sizeof(**allocs));

        for (rmd_u8 i = 0u; i < alloc_cnt; ++i)
                free(allocs[i]);

        // free(allocs);

        rose_mem_debugger_terminate();

        return 0;
}
