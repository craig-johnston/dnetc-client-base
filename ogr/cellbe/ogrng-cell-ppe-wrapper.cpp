/* 
 * Copyright distributed.net 1997-2003 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
*/
const char *ogrng_cell_ppe_wrapper_cpp(void) {
return "@(#)$Id: ogrng-cell-ppe-wrapper.cpp,v 1.5 2008/11/24 20:59:53 stream Exp $"; }


#include <libspe2.h>
#include <cstdlib>
#include <cstring>
#include "unused.h"
#include "logstuff.h"

#define OGR_NG_GET_DISPATCH_TABLE_FXN    spe_ogrng_get_dispatch_table
#define OGROPT_HAVE_OGR_CYCLE_ASM     2

#include "ogrng-cell.h"

#ifndef CORE_NAME
#error CORE_NAME undefined.
#endif

#define SPE_WRAPPER_FUNCTION(name) SPE_WRAPPER_FUNCTION2(name)
#define SPE_WRAPPER_FUNCTION2(name) ogrng_cycle_ ## name ## _spe_wrapper

extern spe_program_handle_t SPE_WRAPPER_FUNCTION(CORE_NAME);

spe_context_ptr_t ps3_assign_context_to_program(spe_program_handle_t *program);

#ifndef HAVE_MULTICRUNCH_VIA_FORK
  #error Code for fork'ed crunchers only - see static Args buffer below
#endif

// #define INTERNAL_TEST

#ifdef INTERNAL_TEST
#define ogr_cycle_256 ogr_cycle_256_test
#endif

static int ogr_cycle_256(struct OgrState *oState, int *pnodes, const u16 *pchoose)
{
  // Check size of structures, these offsets must match assembly
  STATIC_ASSERT(sizeof(struct OgrLevel) == 8*16);
  STATIC_ASSERT(sizeof(struct OgrState) == 2*16 + 8*16*29); /* 29 == OGR_MAXDEPTH */
  STATIC_ASSERT(sizeof(CellOGRCoreArgs) == sizeof(struct OgrState) + 16 + 16 + 16);
  STATIC_ASSERT(sizeof(struct OgrState) <= OGRNG_PROBLEM_SIZE);
  STATIC_ASSERT(offsetof(CellOGRCoreArgs, state.Levels) == 32);
  STATIC_ASSERT(sizeof(pchoose) == 4); /* pchoose cast to u32 */

  static void* myCellOGRCoreArgs_void; // Dummy variable to avoid compiler warnings

  spe_context_ptr_t context;
  unsigned int      entry = SPE_DEFAULT_ENTRY;
  spe_stop_info_t   stop_info;
  int               retval;
  unsigned          thread_index = 99; // todo. enough hacks.

  if ((u32)pchoose & 15)
  {
    Log("OGRNG-SPE#%d! pchoose misaligned (0x%p)\n", thread_index, pchoose);
    abort();
  }

  if (myCellOGRCoreArgs_void == NULL)
  {
    if (posix_memalign(&myCellOGRCoreArgs_void, 128, sizeof(CellOGRCoreArgs)))
    {
      Log("OGRNG-SPE#%d! posix_memalign() failed\n", thread_index);
      abort();
    }
  }

  CellOGRCoreArgs* myCellOGRCoreArgs = (CellOGRCoreArgs*)myCellOGRCoreArgs_void;

  // Copy function arguments to CellOGRCoreArgs struct
  memcpy(&myCellOGRCoreArgs->state, oState, sizeof(struct OgrState));
          myCellOGRCoreArgs->pnodes   = *pnodes;
	  myCellOGRCoreArgs->upchoose = (u32)pchoose;
	  
#ifdef GET_CACHE_STATS
	  myCellOGRCoreArgs->cache_misses = 
	  myCellOGRCoreArgs->cache_hits   =
	  myCellOGRCoreArgs->cache_purges =
	  myCellOGRCoreArgs->cache_search_iters = 
	  myCellOGRCoreArgs->cache_maxlen =
	  myCellOGRCoreArgs->cache_curlen =
	  0;
#endif

  context = ps3_assign_context_to_program(&SPE_WRAPPER_FUNCTION(CORE_NAME));
  retval  = spe_context_run(context, &entry, 0, (void*)myCellOGRCoreArgs, NULL, &stop_info);
  if (retval != 0)
  {
    Log("OGRNG-SPE#%d: spe_context_run() returned %d\n", thread_index, retval);
    abort();
  }

  __asm__ __volatile__ ("sync" : : : "memory");

  // Fetch return value of the SPE core
  if (stop_info.stop_reason == SPE_EXIT)
    retval = stop_info.result.spe_exit_code;
  else
  {
    Log("Alert: OGRNG-SPE#%d exit status is %d\n", thread_index, stop_info.stop_reason);
    abort();
  }

#ifdef GET_CACHE_STATS
  {
  unsigned safe_nodes = myCellOGRCoreArgs->pnodes ? myCellOGRCoreArgs->pnodes : 1;
	
  Log("nodes: %u/%u, hits: %u (%.2f%%), missed: %u\n",
       myCellOGRCoreArgs->pnodes, *pnodes,
       myCellOGRCoreArgs->cache_hits,
       (double)myCellOGRCoreArgs->cache_hits / safe_nodes * 100,
       myCellOGRCoreArgs->cache_misses
     );
  Log(".  purges: %u (%.2f%%), searches: %u (%.2f per node)\n",
       myCellOGRCoreArgs->cache_purges,
       (double)myCellOGRCoreArgs->cache_purges / safe_nodes * 100,
       myCellOGRCoreArgs->cache_search_iters,
       (double)myCellOGRCoreArgs->cache_search_iters / safe_nodes
     );
  Log(".  cache storage usage: %u of %u (%.2f%%)\n", 
       myCellOGRCoreArgs->cache_curlen,
       myCellOGRCoreArgs->cache_maxlen,
       (double)myCellOGRCoreArgs->cache_curlen / (myCellOGRCoreArgs->cache_maxlen ? myCellOGRCoreArgs->cache_maxlen : 1) * 100
     );
  }
#endif

  // Copy data from CellCoreArgs struct back to the function arguments
  memcpy(oState, &myCellOGRCoreArgs->state, sizeof(struct OgrState));
        *pnodes = myCellOGRCoreArgs->pnodes;

  return myCellOGRCoreArgs->ret_depth;
}

#ifdef INTERNAL_TEST
#undef ogr_cycle_256
#include <time.h>
/*
 * Test for direct DMA fetch of pchoose
 */
static int ogr_cycle_256(struct OgrState *state, int *pnodes, const u16 *pchoose)
{
   u32 i;

#if 0
   for (i = 0; i < 128; i++, pchoose++)
   {
     int ret = ogr_cycle_256_test(state, pnodes, pchoose);
     printf("Expected = %u, got = %d%s\n", *pchoose, ret, (*pchoose == ret ? "" : " ** error **"));
   }
#endif
#if 0
   for (i = 0; i < 33; i++)
   {
     unsigned ret, expected = 0xFFFFFFFF >> i;
     *pnodes = i;
     ret = ogr_cycle_256_test(state, pnodes, pchoose);
     printf("%08X %08X%s\n", ret, expected, (ret == expected ? "" : " ** error **"));
   }
#endif     
#if 1
#define START_VALUE  0xFFF00000
   time_t start_time = time(NULL);
   for (i = START_VALUE;;)
   {
     unsigned ret, expected;
     
     *pnodes  = i;
     ret      = ogr_cycle_256_test(state, pnodes, pchoose);
     expected = LOOKUP_FIRSTBLANK(i);
     if (ret != expected)
       printf("Error: input = %u, ret = %u, expected = %u\n", i, ret, expected);
     else
     {
       if ((i & (0x10000 - 1)) == 0)
          printf("%08X iterations OK\n", i);
     }
     if (++i == START_VALUE + 0x100000)
       break;
   }
   printf("SPU call ratio: %u runs/sec.\n", (i-START_VALUE) / (time(NULL)-start_time));
#endif
   exit(0);
}
#endif
