/*
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * Created by Cyrus Patel <cyp@fb14.uni-mainz.de>
 *
 * This module contains hardware identification stuff.
 * See notes on implementing __GetRawProcessorID() below.
 *
*/
const char *cpucheck_cpp(void) {
return "@(#)$Id: cpucheck.cpp,v 1.114.2.9 2003/01/16 09:26:41 oliver Exp $"; }

#include "cputypes.h"
#include "baseincs.h"  // for platform specific header files
#include "cpucheck.h"  //just to keep the prototypes in sync.
#include "logstuff.h"  //LogScreen()/LogScreenRaw()

#if (CLIENT_OS == OS_DEC_UNIX)
#  include <unistd.h>
#  include <sys/sysinfo.h>
#  include <machine/hal_sysinfo.h>
#  include <machine/cpuconf.h>
#elif (CLIENT_OS == OS_MACOS)
#  include <Gestalt.h>
#  include <Multiprocessing.h>
#elif (CLIENT_OS == OS_AIX)
#  include <sys/systemcfg.h>
#elif (CLIENT_OS == OS_MACOSX) && !defined(__RHAPSODY__)
#  include <mach/mach.h>
#elif (CLIENT_OS == OS_DYNIX)
#  include <sys/tmp_ctl.h>
#endif

/* ------------------------------------------------------------------------ */
/*
   Implementing long __GetRawProcessorID( const char **cpuname ):
   
   if identification failed:           return ID==-1L, and set cpuname
                                       to NULL 
   if identification is not supported: return ID==-2L, and set cpuname
                                       to NULL 
   if we have a name, but no ID:       return ID==0, set cpuname to the 
                                       raw name (eg "PCA56" )
   if we have an ID and a name:        return ID and fully formatted 
                                       name (eg "Alpha EV5.6 (21164PC)")
   if we have an ID but no name:       return ID, set cpuname to ""
                                                   -  cyp April/03/1999
*/

/* ------------------------------------------------------------------------ */

int GetNumberOfDetectedProcessors( void )  //returns -1 if not supported
{
  static int cpucount = -2;

  if (cpucount == -2)
  {
    cpucount = -1;
    #if (CLIENT_OS == OS_FREEBSD) || (CLIENT_OS == OS_BSDOS) || \
        (CLIENT_OS == OS_OPENBSD) || (CLIENT_OS == OS_NETBSD)
    { /* comment out if inappropriate for your *bsd - cyp (25/may/1999) */
      int ncpus; size_t len = sizeof(ncpus);
      int mib[2]; mib[0] = CTL_HW; mib[1] = HW_NCPU;
      if (sysctl( &mib[0], 2, &ncpus, &len, NULL, 0 ) == 0)
      //if (sysctlbyname("hw.ncpu", &ncpus, &len, NULL, 0 ) == 0)
        cpucount = ncpus;
    }
    #elif (CLIENT_OS == OS_MACOSX) && !defined(__RHAPSODY__)
    {
      unsigned int    count;
      struct host_basic_info  info;
      count = HOST_BASIC_INFO_COUNT;
      if (host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&info,
          &count) == KERN_SUCCESS)
         cpucount=info.avail_cpus;
    }
    #elif (CLIENT_OS == OS_HPUX) && defined(OS_SUPPORTS_SMP)
    {                          //multithreaded clients are special
      struct pst_dynamic psd;
      if (pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) !=-1)
      cpucount = (int)psd.psd_proc_cnt;
    }
    #elif (CLIENT_OS == OS_BEOS)
    {
      system_info the_info;
      get_system_info(&the_info);
      cpucount = the_info.cpu_count;
    }
    #elif (CLIENT_OS == OS_WIN32)
    {
      SYSTEM_INFO systeminfo;
      GetSystemInfo(&systeminfo);
      cpucount = systeminfo.dwNumberOfProcessors;
      if (cpucount < 1)
        cpucount = -1;
    }
    #elif (CLIENT_OS == OS_NETWARE)
    {
      cpucount = GetNumberOfRegisteredProcessors();
    }
    #elif (CLIENT_OS == OS_OS2)
    {
      int rc = (int) DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS,
                  &cpucount, sizeof(cpucount));
      if (rc != 0 || cpucount < 1)
        cpucount = -1;
    }
    #elif (CLIENT_OS == OS_LINUX) || (CLIENT_OS == OS_PS2LINUX)
    {
      #if (CLIENT_CPU == CPU_ARM) || (CLIENT_CPU == CPU_MIPS)
        cpucount = 1;
      #else
      FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
      cpucount = 0;
      if ( cpuinfo )
      {
        char buffer[256];
        while(fgets(buffer, sizeof(buffer), cpuinfo))
        {
          buffer[sizeof(buffer) - 1] = '\0';
          #if (CLIENT_CPU == CPU_X86      || \
               CLIENT_CPU == CPU_POWERPC  || \
               CLIENT_CPU == CPU_S390     || \
               CLIENT_CPU == CPU_S390X    || \
               CLIENT_CPU == CPU_PA_RISC)
          if (strstr(buffer, "processor") == buffer)
            cpucount++;
          #elif (CLIENT_CPU == CPU_SPARC)
          // 2.1.x kernels (at least 2.1.125)
          if (strstr( buffer, "ncpus active\t: " ) == buffer)
            cpucount = atoi( buffer+15 );
          // 2.0.x non-smp kernels (at least 2.0.35)
          else if (strstr( buffer, "BogoMips\t: " ) == buffer)
            cpucount = 1;
          // 2.0.x smp kernels (at least 2.0.35)
          else if ( buffer == strstr( buffer, 
                    "        CPU0\t\tCPU1\t\tCPU2\t\tCPU3\n" ) )
          {
            fgets( buffer, 256, cpuinfo );
            for (char *p = strtok( buffer+7, "\t \n" ); p;
                    p = strtok( NULL, "\t \n" ))
            {
              if (strstr( p, "online" ) || strstr( p, "akp"))
                cpucount++;
            }
          }
          #elif (CLIENT_CPU == CPU_ALPHA)
          cpucount = 1; /* assume this until we know otherwise */
          /* SMP data (2.1+) - "CPUs probed %d active %d map 0x%x AKP %d\n" */
          if (memcmp(buffer, "CPUs probed", 11) == 0 && 
                    (buffer[11] == '\t' || buffer[11] == ' '))
          {
            char *p = strstr( buffer, "active" );
            if (p && (p[6] == '\t' || p[6] == ' '))
            {
              p += 6; while (*p && !isdigit(*p)) p++;
              cpucount = atoi(p);
              break;
            }
          }
          else if (memcmp(buffer, "cpus detected\t",14)==0) /* 2.4.1 */
          {                          /* "cpus detected\t\t: 4" */
            char *p = &buffer[14];
            while (*p == '\t' || *p == ':' || *p == ' ')
              p++;
            if ((p > &buffer[14]) && isdigit(*p))
            {
              cpucount = atoi(p);
              break; 
            }
          }
          #else
          cpucount = -1;
          break;
          #endif
        }
        fclose(cpuinfo);
      }
      #endif // (CLIENT_CPU == CPU_ARM)
    }
    #elif (CLIENT_OS == OS_IRIX)
    {
      cpucount = (int)prctl( PR_MAXPPROCS, 0, 0);
    }
    #elif (CLIENT_OS == OS_SOLARIS)
    {
      cpucount = sysconf(_SC_NPROCESSORS_ONLN);
    }
    #elif (CLIENT_OS == OS_AIX)
    {
      cpucount = sysconf(_SC_NPROCESSORS_ONLN);
      //cpucount = _system_configuration.ncpus; 
      //should work the same way but might go
    }
    #elif (CLIENT_OS == OS_RISCOS)
    {
      cpucount = riscos_count_cpus();
    }
    #elif (CLIENT_OS == OS_QNX) && defined(__QNXNTO__) /* neutrino */
    {
      cpucount = _syspage_ptr->num_cpu;
    }
    #elif (CLIENT_OS == OS_MACOS) && (CLIENT_CPU == CPU_POWERPC)
    {
      cpucount = 1;
      if (MPLibraryIsLoaded())
        cpucount = MPProcessors();
    }
    #elif (CLIENT_OS == OS_AMIGAOS)
    {
      cpucount = 1;
    }
    #elif ( (CLIENT_OS == OS_DEC_UNIX) && defined(OS_SUPPORTS_SMP))
    {
      // We really only want to do this for multithreaded clients.
      // Earlier versions of the Digital Unix don't support this.
      int status = 0;
      struct cpu_info buf;
      int st = 0;
      status = getsysinfo(GSI_CPU_INFO, (char *) &buf, sizeof(buf), st, NULL,NULL);
      if (status == -1)
        cpucount = -1;
      else
        cpucount = buf.cpus_in_box;
    }
    #elif (CLIENT_CPU == CPU_68K) // no such thing as 68k/mp
      cpucount = 1;               // that isn't covered above
    #elif (CLIENT_OS == OS_DYNIX)
      int nprocs = tmp_ctl(TMP_NENG, 0);
      int i;
      cpucount = 0;
      for (i = 0; i < nprocs; i++)
        if (TMP_ENG_ONLINE == tmp_ctl(TMP_QUERY, i)) cpucount++;
    #endif
    if (cpucount < 1)  //not supported
      cpucount = -1;
  }

  return cpucount;
}

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_68K)
static long __GetRawProcessorID(const char **cpuname)
{
  static long detectedtype = -2; /* -1==failed, -2==not supported */
  static const char *detectedname = NULL;
  static char namebuf[20];
  static struct { const char *name; long rid; } cpuridtable[] = {
                { "68000",           68000L  },
                { "68010",           68010L  },
                { "68020",           68020L  },
                { "68030",           68030L  },
                { "68040",           68040L  },
                { "68060",           68060L  }
                };

  #if (CLIENT_OS == OS_AMIGAOS)
  if (detectedtype == -2)
  {
    long flags = (long)(SysBase->AttnFlags);

    if ((flags & AFF_68060) && (flags & AFF_68040) && (flags & AFF_68030))
      detectedtype = 68060L; // Phase5 060 boards at least report this...
    else if ((flags & AFF_68040) && (flags & AFF_68030))
      detectedtype = 68040L; // 68040
    else if ((flags & AFF_68030) && (flags & AFF_68020))
      detectedtype = 68030L; // 68030
    else if ((flags & AFF_68020) && (flags & AFF_68010))
      detectedtype = 68020L; // 68020
    else if (flags & AFF_68010)
      detectedtype = 68010L; // 68010
    else
      detectedtype = 68000L; // 68000
  }
  #elif (CLIENT_OS == OS_MACOS)
  if (detectedtype == -2)
  {
    long result;
    // Note: gestaltProcessorType is used so that if the 68K client is run on
    // on PPC machine for test purposes, it will get the type of the 68K
    // emulator. Also, gestaltNativeCPUType is not present in early versions of
    // System 7. (For more info, see Gestalt.h)
    detectedtype = -1L;
    if (Gestalt(gestaltProcessorType, &result) == noErr)
    {
      switch(result)
      {
        case gestalt68000: detectedtype = 68000L;  break;
        case gestalt68010: detectedtype = 68010L;  break;
        case gestalt68020: detectedtype = 68020L;  break;
        case gestalt68030: detectedtype = 68030L;  break;
        case gestalt68040: detectedtype = 68040L;  break;
        default:           detectedtype = -1L;     break;
      }
    }
  }
  #elif (CLIENT_OS == OS_LINUX)
  if (detectedtype == -2L)
  {
    FILE *cpuinfo;
    detectedtype = -1L;
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      while(fgets(buffer, sizeof(buffer), cpuinfo)) 
      {
        const char *p = "CPU:\t\t";
        unsigned int n = strlen( p );
        if ( memcmp( buffer, p, n ) == 0 )
        {
          p = &buffer[n]; buffer[sizeof(buffer)-1]='\0';
          for ( n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
          {
            unsigned int l = strlen( cpuridtable[n].name );
            if ((!p[l] || isspace(p[l])) && memcmp(p,cpuridtable[n].name,l)==0)
            {
              detectedtype = cpuridtable[n].rid;
              break;
            }
          }
          if (detectedtype == -1L)
          {
            for ( n = 0; *p && *p!='\r' && *p!='\n' && n<(sizeof(namebuf)-1); n++ )
              namebuf[n] = *p++;
            namebuf[n] = '\0';
            detectedname = (const char *)&namebuf[0];
            detectedtype = 0;
          }
          break;
        }
      }
      fclose(cpuinfo);
    }
  }
  #endif

  if (detectedtype > 0 && detectedname == NULL )
  {
    unsigned int n;
    detectedname = "";
    for (n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
    {
      if (((long)(cpuridtable[n].rid)) == detectedtype )
      {
        strcpy( namebuf, "Motorola " );
        strcat( namebuf, cpuridtable[n].name );
        detectedname = (const char *)&namebuf[0];
        break;
      }
    }
  }

  if (cpuname)
    *cpuname = detectedname;
  return detectedtype;
}

#endif /* (CLIENT_CPU == CPU_68K) */

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_POWERPC) || (CLIENT_CPU == CPU_POWER)
static long __GetRawProcessorID(const char **cpuname)
{
  /* ******* detected type reference is (PVR value >> 16) *********** */
  static long detectedtype = -2L; /* -1 == failed, -2 == not supported */
  static int ispower = 0, isaltivec = 0;
  static const char *detectedname = NULL;
  static char namebuf[30];
  static struct { long rid; const char *name; } cpuridtable[] = {
    //note: if the name is not prefixed with "Power", it defaults to "PowerPC"
    //note: Non-PVR based numbers start at 0x10000 (real PVR numbers are 16bit)
    //note: always use PVR IDs http://e-www.motorola.com/collateral/PPCPVR.pdf
                {    0x0001,   "601"                   },
                {    0x0003,   "603"                   },
                {    0x0004,   "604"                   },
                {    0x0006,   "603e"                  },
                {    0x0007,   "603r/603ev"            }, //ev=0x0007, r=0x1007
                {    0x0008,   "740/750 (G3)"          },
                {    0x0009,   "604e"                  },
                {    0x000A,   "604ev"                 },
                {    0x000C,   "7400 (G4)"             },
                {    0x0020,   "403G/403GC/403GCX"     },
                {    0x0050,   "821"                   },
                {    0x0080,   "860"                   },
                {    0x0081,   "8240"                  },
                {    0x4011,   "405GP"                 },
                {    0x8000,   "7450 (G4)"             },
                {    0x8001,   "7455 (G4)"             },
                {    0x800C,   "7410 (G4)"             },
                {(1L<<16)+1,   "Power RS"              }, //not PVR based
                {(1L<<16)+2,   "Power RS2 Superchip"   }, //not PVR based
                {(1L<<16)+3,   "Power RS2"             }, //not PVR based
                {(1L<<16)+4,   "620"                   }, //not PVR based
                {(1L<<16)+5,   "630"                   }, //not PVR based
                {(1L<<16)+6,   "A35"                   }, //not PVR based
                {(1L<<16)+7,   "RS64II"                }, //not PVR based
                {(1L<<16)+8,   "RS64III"               }, //not PVR based
                };
  #if (CLIENT_OS == OS_AIX)
  if (detectedtype == -2L)
  { 
    static struct { long imp;   long rid; } cpumap[] = {
    /* imp constants from src/bos/kernel/sys/POWER/systemcfg.h 1.12 */
                  { 0x0008   ,            1 }, /* POWER_601 */
                  { 0x0020   ,            3 }, /* POWER_603 */
                  { POWER_604,            4 }, /* POWER_604 */
                  { 0x0001   ,   (1L<<16)+1 }, /* POWER_RS1 */
                  { 0x0002   ,   (1L<<16)+2 }, /* POWER_RSC */
                  { 0x0004   ,   (1L<<16)+3 }, /* POWER_RS2 */
                  { 0x0040   ,   (1L<<16)+4 }, /* POWER_620 */
                  { 0x0080   ,   (1L<<16)+5 }, /* POWER_630 */
                  { 0x0100   ,   (1L<<16)+6 }, /* POWER_A35 */
                  { 0x0200   ,   (1L<<16)+7 }, /* POWER_RS64II */
                  { 0x0400   ,   (1L<<16)+8 }, /* POWER_RS64III */
                  };
    unsigned int imp_i;
    detectedtype = -1L; /* assume failed */
    if ( _system_configuration.architecture == POWER_RS ) 
      ispower = 1;
    for (imp_i = 0; imp_i < (sizeof(cpumap)/sizeof(cpumap[0])); imp_i++)
    {
      if (cpumap[imp_i].imp == _system_configuration.implementation )
      {
        detectedtype = cpumap[imp_i].rid;
        break;
      }
    }
    if (detectedtype == -1L) /* ident failed */
    {
      sprintf( namebuf, "impl:0x%lX", _system_configuration.implementation );
      detectedname = (const char *)&namebuf[0];
      if (ispower) /* if POWER CPU, then don't let ident fail */
      {            /*   - we need the power bit in the retval */
        detectedtype = (1L<<16)+_system_configuration.implementation;
      }
    }
  }
  #elif (CLIENT_OS == OS_MACOS)
  if (detectedtype == -2L)
  {
    // Note: need to use gestaltNativeCPUtype in order to get the correct
    // value for G3 upgrade cards in a 601 machine.
    // PVR is a hardware value from the cpu and is available on every 
    // PPC CPU on every PPC Based OS.
    long result;
    detectedtype = -1;
    if (Gestalt(gestaltNativeCPUtype, &result) == noErr)
    {
      /* Fix Gestalt IDs to match pure PVR values */
      if (result == gestaltCPUG47450) /* gestaltCPUG47450 = 0x0110 */
        result = 0x8100L; /* Apples ID makes sense but we prefer pure PVR */
      if (result == 0x0111) /* gestaltCPUG47455 = 0x0111 */
        result = 0x08101L; /* Apples ID makes sense but we prefer pure PVR */
      detectedtype = result - 0x100L; // PVR!!

      /* AltiVec software support and hardware support/exsitence */
      if (Gestalt( gestaltSystemVersion, &result ) == noErr)
      {   
        if (result >= 860) /* Mac OS 8.6 and above? */
        {
          if ( Gestalt(gestaltPowerPCProcessorFeatures, &result) == noErr)
          {
            if (((1 << gestaltPowerPCHasVectorInstructions) & result)!=0)
              isaltivec = 1;
          }
        }
      }
    }
  }
  #elif (CLIENT_OS == OS_MACOSX) && !defined(__RHAPSODY__)
  if (detectedtype == -2L)
  {
    kern_return_t   kr;
    unsigned int    count;
    struct host_basic_info  info;

    count = HOST_BASIC_INFO_COUNT;
    kr = host_info(mach_host_self(),HOST_BASIC_INFO,(host_info_t)&info,&count);
    if (kr == KERN_SUCCESS)
    {
      // host_info() doesnt use PVR values, so I map them as in mach/machine.h
      // http://www.opensource.apple.com/cgi-bin/registered/cvs/System/xnu/osfmk/mach/
      switch (info.cpu_subtype)
      {
        case CPU_SUBTYPE_POWERPC_601:  detectedtype = 0x0001; break;
        case CPU_SUBTYPE_POWERPC_603:  detectedtype = 0x0003; break;
        case CPU_SUBTYPE_POWERPC_603e: detectedtype = 0x0006; break;
        case CPU_SUBTYPE_POWERPC_603ev:detectedtype = 0x0007; break;
        case CPU_SUBTYPE_POWERPC_604:  detectedtype = 0x0004; break;
        case CPU_SUBTYPE_POWERPC_604e: detectedtype = 0x0009; break;
        case CPU_SUBTYPE_POWERPC_750:  detectedtype = 0x0008; break;
        case CPU_SUBTYPE_POWERPC_7400: detectedtype = 0x000C; break;
        case CPU_SUBTYPE_POWERPC_7450: detectedtype = 0x8000; break;
        default: // some PPC processor that we don't know about
                 // set the tag (so that the user can tell us), but return 0
        sprintf(namebuf, "0x%x", info.cpu_subtype );
        detectedname = (const char *)&namebuf[0];
        detectedtype = 0;
        break;
      }
    }
    // AltiVec support now has a proper sysctl value HW_VECTORUNIT to check for
    int mib[2], hasVectorUnit; mib[0] = CTL_HW; mib[1] = HW_VECTORUNIT;
    size_t len = sizeof(hasVectorUnit);
    if (sysctl( mib, 2, &hasVectorUnit, &len, NULL, 0 ) == 0)
    {
      if (hasVectorUnit != 0)
        isaltivec = 1;
    }
  }
  #elif (CLIENT_OS == OS_WIN32)
  if (detectedtype == -2L)
  {
    SYSTEM_INFO si;
    si.wProcessorArchitecture = 0;
    si.wProcessorRevision = si.wProcessorLevel = 0;
    detectedtype = -1L;
    GetSystemInfo( &si );
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC)
    {
      detectedtype = si.wProcessorRevision;
      if (detectedtype == 0)
        detectedtype = si.wProcessorLevel; 
      if (detectedtype == 0)
        detectedtype = -1;
    }
  }
  #elif (CLIENT_OS == OS_LINUX)
  if (detectedtype == -2L)
  {
    FILE *cpuinfo;
    detectedtype = -1L;
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      while(fgets(buffer, sizeof(buffer), cpuinfo)) 
      {
        const char *p = "cpu\t\t: ";
        unsigned int n = strlen( p );
        if ( memcmp( buffer, p, n ) == 0 )
        {
          static struct 
           { const char *sig;  int rid; } sigs[] = {
           /*
           last update Jun 13/2000 from 
           http://lxr.linux.no/source/arch/ppc/kernel/setup.c?v=2.3.99-pre5;a=ppc
           */
           { "601",             0x0001  },
           { "603",             0x0003  },
           { "604",             0x0004  },
           { "603e",            0x0006  },
           { "603ev",           0x0007  },
           { "603r",            0x0007  },
           { "740/750",         0x0008  },
           { "750",             0x0008  },
           { "750CX",           0x0008  },
           { "750P",            0x0008  },
           { "604e",            0x0009  },
           { "604r",            0x000A  }, /* >= 2.3.99 */
           { "604ev",           0x000A  }, /* < 2.3.34 */
           { "604ev5",          0x000A  }, /* >= 2.3.34 */
           { "7400",            0x000C  },
           { "403G",            0x0020  },
           { "403GC",           0x0020  },
           { "403GCX",          0x0020  },
           { "821",             0x0050  },
           { "860",             0x0080  },
           { "8240",            0x0081  },
           { "405GP",           0x4011  }
           };
          p = &buffer[n]; buffer[sizeof(buffer)-1]='\0';
          for ( n = 0; n < (sizeof(sigs)/sizeof(sigs[0])); n++ )
          {
            unsigned int l = strlen( sigs[n].sig );
            if ((!p[l] || isspace(p[l])) && memcmp( p, sigs[n].sig, l)==0)
            {
              detectedtype = (long)sigs[n].rid;
              break;
            }
          }
          if (detectedtype == 0x000C) /* 7400 (G4) */
          {
            /* Altivec support appeared in 2.3.99-pre4 */
            struct utsname ut;
            if (uname(&ut)==0)
            {
              if (strcmp(ut.sysname,"Linux")==0)
              {
                p = strchr(ut.release,'.');
                n = atoi(ut.release);
                if (n < 2)
                  isaltivec = 0;
                else if (n > 2)
                  isaltivec = 1;
                else if (!p)
                  isaltivec = 0;
                else if (atoi(p+1) >= 4)
                  isaltivec = 1;
              }
            }
          }
          if (detectedtype == -1L)
          {
            for ( n = 0; *p && *p!='\n' && *p!='\r' && n<(sizeof(namebuf)-1); n++ )
              namebuf[n] = *p++;
            namebuf[n] = '\0';
            detectedname = (const char *)&namebuf[0];
            detectedtype = 0;
          }
          break;
        }
      }
      fclose(cpuinfo);
    }
  }
  #elif (CLIENT_OS == OS_BEOS)  // BeOS PPC
  if (detectedtype == -2L)
  {
    system_info sInfo;
    sInfo.cpu_type = (cpu_type) 0;
    get_system_info(&sInfo);
    detectedtype = -1;
    if (sInfo.cpu_type) /* didn't fail */
    {
      switch (sInfo.cpu_type)
      {
        case B_CPU_PPC_601:  detectedtype = 0x0001; break;
        case B_CPU_PPC_603:  detectedtype = 0x0003; break;
        case B_CPU_PPC_603e: detectedtype = 0x0006; break;
        case B_CPU_PPC_604:  detectedtype = 0x0004; break;
        case B_CPU_PPC_604e: detectedtype = 0x0009; break;
        case B_CPU_PPC_750:  detectedtype = 0x0008; break;
        default: // some PPC processor that we don't know about
                 // set the tag (so that the user can tell us), but return 0
        sprintf(namebuf, "0x%x", sInfo.cpu_type );
        detectedname = (const char *)&namebuf[0];
        detectedtype = 0;
        break;
      }
    }
  }
  #elif (CLIENT_OS == OS_AMIGAOS)  // AmigaOS PPC
  if (detectedtype == -2L)
  {
    #ifndef __POWERUP__
    /* WarpOS */
    struct TagItem cputags[2] = { {GETINFO_CPU, 0}, {TAG_END,0} };
    GetInfo(cputags);
    switch (cputags[0].ti_Data)
    {
      case CPUF_603:  detectedtype = 0x0003; break;
      case CPUF_603E: detectedtype = 0x0006; break;
      case CPUF_604:  detectedtype = 0x0004; break;
      case CPUF_604E: detectedtype = 0x0009; break;
      case CPUF_620:  detectedtype = 0x0004 + (1<<16); break;
      case CPUF_G3:   detectedtype = 0x0008; break;
      case CPUF_G4:   detectedtype = 0x8000; break;
      default: // some PPC processor that we don't know about
               // set the tag (so that the user can tell us), but return 0
      sprintf(namebuf, "WOS:0x%lx", cputags[0].ti_Data );
      detectedname = (const char *)&namebuf[0];
      detectedtype = 0;
      break;
    }
    #else
    /* PowerUp */
    ULONG cpu = PPCGetAttr(PPCINFOTAG_CPU);
    switch (cpu)
    {
      case CPU_603:  detectedtype = 0x0003; break;
      case CPU_603e: detectedtype = 0x0006; break;
      case CPU_603p: detectedtype = 0x0006; break;
      case CPU_604:  detectedtype = 0x0004; break;
      case CPU_604e: detectedtype = 0x0009; break;
      case 0x0008:   // G3
      case 0x000C:   // 7400 (G4)
      case 0x8000:   // 7450 (G4)
      case 0x8001:   // 7455 (G4)
      case 0x800C:   // 7410 (G4)
      detectedtype = cpu; break;
      default: // some PPC processor that we don't know about
               // set the tag (so that the user can tell us), but return 0
      sprintf(namebuf, "PUP:0x%lx", cpu );
      detectedname = (const char *)&namebuf[0];
      detectedtype = 0;
      break;
    }
    #endif
  }
  #endif
  
  if (detectedtype > 0 && detectedname == NULL )
  {
    unsigned int n;
    detectedname = "";
    for (n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
    {
      if (cpuridtable[n].rid == detectedtype )
      {
        if (strlen( cpuridtable[n].name )>=6 &&
            memcmp( cpuridtable[n].name, "Power ", 6 )==0)
          namebuf[0] = '\0';
        else
          strcpy( namebuf, "PowerPC ");
        strcat( namebuf, cpuridtable[n].name );
        detectedname = (const char *)&namebuf[0];
        break;
      }
    }
  }
  
  if (cpuname)
    *cpuname = detectedname;
  if (detectedtype > 0)
  {
    if (ispower)    
      return ((1L<<24)|detectedtype);
    if (isaltivec) /* *OS* supports altivec */
      return ((1L<<25)|detectedtype);
  }
  return detectedtype;
}
#endif /* (CLIENT_CPU == CPU_POWERPC) || (CLIENT_CPU == CPU_POWER) */

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_X86)
static u32 __os_x86ident_fixup(u32 x86ident_result)
{
  #if (CLIENT_OS == OS_LINUX)
  if (x86ident_result == 0x79430400) /* Cyrix indeterminate */
  {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (file)
    {
      int vendor_id = 0, family = 0, model = 0;
      char buf[128]; 
      while (fgets(buf, sizeof(buf)-1, file))
      {
        char *p; int c;
        buf[sizeof(buf)-1] = '\0';
        p = strchr(buf, '\n');
        if (p) 
          *p = '\0';
        else
        {
          c = 1;
          while (c != EOF && c != '\n')
            c = fgetc(file);
          p = &buf[sizeof(buf-1)]; /* "" */
        }      
        c = 0;
        while (buf[c] && buf[c] != ':')
          c++;
        if (buf[c] == ':') 
          p = &buf[c+1];
        while (c > 0 && (buf[c-1]==' ' || buf[c-1] == '\t'))
          c--;
        buf[c] = '\0';
        while (*p == ' ' || *p == '\t')
          p++;
        c = 0;
        /* printf("key='%s', val='%s'\n", buf, p); */
        while (p[c] && p[c] != ' ' && p[c] != '\t')
          c++;
        p[c] = '\0';  

        if (strcmp(buf,"vendor_id") == 0)
        {
          if (strcmp(p,"CyrixInstead") == 0) /* we only care about this one */
            vendor_id = 0x7943;
          else
            break;
        }  
        else if (strcmp(buf, "model name")==0)
        {
          if (strncmp(p, "5x86", 4)!=0)
            break;
          family = 4; model = 9; 
          /* linux simulates 5x86 as fam=4,mod=1,step=5, x86ident() as 4,9,x */
        }  
      }
      fclose(file);
      if (vendor_id == 0x7943 && family == 4 && model == 9)
        return 0x79430490;
    } /* if (file) */
  } /* if (cyrix indeterminate) */
  #endif /* (CLIENT_OS == OS_LINUX) */
  return x86ident_result;
}  

#if (CLIENT_OS == OS_LINUX) && !defined(__ELF__)
  extern "C" u32 x86ident( void ) asm ("x86ident");
#else
#if defined(__WATCOMC__)
  // x86ident() can destroy all registers except ebx/esi/edi/ebp =>
  // must be declared as "cdecl" to allow compiler save necessary registers.
  extern "C" u32 __cdecl x86ident( void );
#else
  extern "C" u32 x86ident( void );
#endif
  extern "C" u32 x86ident_haveioperm; /* default is zero */
#endif

// whattoret 0:detectedtype, 'c':simpleid, 'f':featureflags
long __GetRawProcessorID(const char **cpuname, int whattoret = 0 )
{
  static long detectedtype = -2L;  /* -1 == failed, -2 == not supported */
  static const char *detectedname = NULL;
  static int simpleid = 0xff;      /* default id if not found */
  static int featureflags = 0;     /* default flags if id not found */
  
  if ( detectedtype == -2L )
  {
    static char namebuf[80];
    const char *vendorname = NULL;
    const char *chipname_override = NULL;
    int cpuidbmask, cpuid, vendorid; u32 dettype; 
    struct cpuxref { int cpuid, cpufeatures, simpleid; 
                     const char *cpuname; } *internalxref = NULL;

    #if (CLIENT_OS == OS_WIN32) || (CLIENT_OS == OS_WIN16)
    if (winGetVersion() < 2000) /* win95 permits inb()/outb() */
      x86ident_haveioperm = 1;
    #elif (CLIENT_OS == OS_DOS) || (CLIENT_OS == OS_NETWARE) 
      x86ident_haveioperm = 1;        /* netware client runs at IOPL 0 */
    #elif (CLIENT_OS == OS_LINUX)
    //if (x86ident() == 0x79430400 && geteuid()==0)
    //  x86ident_haveioperm = (ioperm(0x22, 2, 1)==0);
    #elif (CLIENT_OS == OS_FREEBSD) || (CLIENT_OS == OS_OPENBSD) || \
          (CLIENT_OS == OS_NETBSD)
    //if (x86ident() == 0x79430400 && geteuid()==0)
    //  x86ident_haveioperm = (i386_set_ioperm(0x22, 2, 1)==0);
    #endif

    dettype     = __os_x86ident_fixup(x86ident());
    cpuidbmask  = 0xfff0; /* mask with this to find it in the table */
    cpuid       = (((int)dettype) & 0xffff);
    vendorid    = (((int)(dettype >> 16)) & 0xffff);

    if (vendorid == 0x6E49 ) /* 'nI': broken x86ident */
      vendorid = 0x6547; /* 'eG' */
  
    sprintf( namebuf, "%04X:%04X", vendorid, cpuid );
    detectedname = (const char *)&namebuf[0];
    if (vendorid || cpuid)
      detectedtype = 0; /* allow tag to be shown */
    else
      detectedtype = -1; /* assume not found */
    simpleid = 0xff; /* default id = unknown */
    
    /* DO *NOT* guess. use detype=0xFF when you don't know what to use. */
    if ((cpuid & 0xff00) == 0x0000)       /* generic legacy stuff */
    {                                     /* new x86ident splits completely */
      static struct cpuxref genericxref[]={
          {  0x0030, CPU_F_I386,       1, "80386"     },   // generic 386/486 core
          {  0x0040, CPU_F_I486,       1, "80486"     },
          {  0x0000, 0,               -1, NULL        }
          }; internalxref = &genericxref[0];
      vendorname = ""; /* nothing */
      cpuidbmask = 0xfff0;
    }
    else if ( vendorid == 0x4D54 /* 'MT' NOTE! */) /* "GenuineTMx86" */
    {
      static struct cpuxref transmeta_xref[]={
          {  0x0320, CPU_F_I386,       0, "TM3200"    }, /* 0x320 is a guess */ /* I386 is a guess */
          {  0x0540, CPU_F_I586,       0, "TM5400"    },
          {  0x0000, 0,               -1, NULL        }
          }; internalxref = &transmeta_xref[0];
      vendorname = "Transmeta";
      cpuidbmask = 0x0ff0;
    }
    else if ( vendorid == 0x7943 /* 'yC' */ ) /* CyrixInstead */
    {                                  
      static struct cpuxref cyrixxref[]={
          {  0x0400, CPU_F_I486,       6, "486SLC/DLC/SR/DR" },
          {  0x0410, CPU_F_I486,       6, "486S/Se/S2/DX/DX2" },
          {  0x0440, CPU_F_I486,       0, "GX/MediaGX" },
          {  0x0490, CPU_F_I486,       0, "5x86"      },
          {  0x0520, CPU_F_I586,       3, "6x86/MI"   },
          {  0x0540, CPU_F_I586,       0, "GXm"       },
          {  0x0600, CPU_F_I686MMX,    9, "6x86MX/MII"},
          /* The VIA Cyrix III is CentaurHauls */
          {  0x0000, 0,               -1, NULL        }
          }; internalxref = &cyrixxref[0]; 
      vendorname = "Cyrix";
      cpuidbmask = 0x0ff0;
    }
    else if ( vendorid == 0x6F43 /* 'eM' */)
    {
      static struct cpuxref vpc[]={
          {  0x0535, CPU_F_I586,       0, "VPC586" },
          {  0x0000, 0,               -1, NULL     }
          }; internalxref = &vpc[0];
      vendorname = "Connectix";
      cpuidbmask = 0x0fff;
    }
    else if ( vendorid == 0x6543 /* 'eC' */ ) /* "CentaurHauls" */
    {
      static struct cpuxref centaurxref[]={
          {  0x0540, CPU_F_I586MMX, 0x0A, "C6" }, /* has its own id */
          {  0x0580, CPU_F_I586MMX, 0x0A, "C2" }, /* uses RG Cx re-pair */
          {  0x0590, CPU_F_I586MMX, 0x0A, "C3" },
          /* I'm not sure about the following two: are they I586 or I686 ? so do it the safe way */
          {  0x0660, CPU_F_I586MMX, 0x09, "Samuel (Cyrix III)" }, /* THIS IS NOT A P6 !!! */
          {  0x0670, CPU_F_I586MMX, 0x0A, "C3" },
          {  0x0000, 0,               -1, NULL }
          }; internalxref = &centaurxref[0];
      vendorname = "Centaur/IDT";
      if (cpuid >= 0x0600)
        vendorname = "VIA";
      cpuidbmask = 0x0ff0;
    }
    else if ( vendorid == 0x6952 /* 'iR' */  ) /* "RiseRiseRiseRise" */
    {
      static struct cpuxref risexref[]={
          {  0x0500, CPU_F_I586,    0xFF, "mP6" }, /* (0.25 �m) - dunno which core */ /* I586 is a guess */
          {  0x0500, CPU_F_I586,    0xFF, "mP6" }, /* (0.18 �m) - dunno which core */ /* I586 is a guess */
          {  0x0000, 0,               -1, NULL  }
          }; internalxref = &risexref[0];
      vendorname = "Rise";
      cpuidbmask = 0xfff0;
    }
    else if ( vendorid == 0x654E /* 'eN' */  ) //"NexGenDriven"
    {   
      static struct cpuxref nexgenxref[]={
          {  0x0500, CPU_F_I586,       1, "Nx586" }, //386/486 core  /* I586 is a guess */
          {  0x0000, 0,               -1, NULL    } //no such thing
          }; internalxref = &nexgenxref[0];
      vendorname = "NexGen";
      cpuidbmask = 0xfff0;
    }
    else if ( vendorid == 0x4D55 /* 'MU' */  ) //"UMC UMC UMC "
    {   
      static struct cpuxref umcxref[]={
          {  0x0410, CPU_F_I486,       0, "U5D" },
          {  0x0420, CPU_F_I486,       0, "U5S" },
          {  0x0000, 0,               -1, NULL  }
          }; internalxref = &umcxref[0];
      vendorname = "UMC";
      cpuidbmask = 0xfff0;
    }
    else if ( vendorid == 0x7541 /* 'uA' */ ) // "AuthenticAMD"
    {
      /* see "AMD Processor Recognition Application Note" available at
         http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/20734.pdf */
      static struct cpuxref amdxref[]={
          {  0x0400, CPU_F_I486,       0, "486"      },
          {  0x0430, CPU_F_I486,       0, "486DX2"   },
          {  0x0470, CPU_F_I486,       0, "486DX2WB" },
          {  0x0480, CPU_F_I486,       0, "486DX4"   },
          {  0x0490, CPU_F_I486,       0, "486DX4WB" },
          {  0x04E0, CPU_F_I486,       6, "5x86"     },
          {  0x04F0, CPU_F_I486,       6, "5x86WB"   },
          {  0x0500, CPU_F_I586,       4, "K5 PR75, PR90, or PR100" }, // use K5 core
          {  0x0510, CPU_F_I586,       4, "K5 PR120 or PR133" },
          {  0x0520, CPU_F_I586,       4, "K5 PR166" },
          {  0x0530, CPU_F_I586,       4, "K5 PR200" },
          {  0x0560, CPU_F_I586MMX,    5, "K6"       },
          {  0x0570, CPU_F_I586MMX,    5, "K6"       },
          {  0x0580, CPU_F_I586MMX,    5, "K6-2"     },
          {  0x0590, CPU_F_I586MMX,    5, "K6-3"     },
          {  0x05D0, CPU_F_I586MMX,    5, "K6-2+/K6-3+" },
          {  0x0610, CPU_F_I686MMX,    9, "K7 (Athlon)"            }, // slot A
          {  0x0620, CPU_F_I686MMX,    9, "K7-2 (Athlon)"          }, // slot A
          {  0x0630, CPU_F_I686MMX,    9, "K7-3 (Duron)"           }, // 64K L2
          {  0x0640, CPU_F_I686MMX,    9, "K7-4 (Athlon Thunderbird)" }, // 256K L2
          {  0x0660, CPU_F_I686MMX,    9, "K7-6 (Athlon XP/MP/-4)" }, // Palomino core, 256K L2
          {  0x0670, CPU_F_I686MMX,    9, "K7-7 (Duron)"           }, // Morgan core = Palomino core w/ 64K L2
          {  0x0680, CPU_F_I686MMX,    9, "K7-8 (Athlon XP)"       }, // Thoroughbred core
          {  0x0000, 0,               -1, NULL       }
          }; internalxref = &amdxref[0];
      vendorname = "AMD";
      if (cpuid == 0x0400)          /* no such AMD ident */
        vendorname = "Intel/AMD";  /* identifies AMD or Intel 486 */
      cpuidbmask = 0x0ff0;
    }
    else if ( vendorid == 0x6547 /* 'eG' */ ) // "GenuineIntel"
    {
      /* the following information has been collected from the 
         "AP-485 Intel Processor Identification and the CPUID Instruction"
         manual available at 
         http://www.intel.com/design/xeon/applnots/241618.htm
         and several "Intel XYZ Processor Specification Update" documents
         available from http://www.intel.com/design/processor/index.htm */
      static struct cpuxref intelxref[]={
          {  0x0300, CPU_F_I386,       1, "386SX/DX" },
          {  0x0400, CPU_F_I486,       1, "486DX 25 or 33" },
          {  0x0410, CPU_F_I486,       1, "486DX 50" },
          {  0x0420, CPU_F_I486,       1, "486SX" },
          {  0x0430, CPU_F_I486,       1, "486DX2" },
          {  0x0440, CPU_F_I486,       1, "486SL" },
          {  0x0450, CPU_F_I486,       1, "486SX2" },
          {  0x0470, CPU_F_I486,       1, "486DX2WB" },
          {  0x0480, CPU_F_I486,       1, "486DX4" },
          {  0x0490, CPU_F_I486,       1, "486DX4WB" },
          {  0x0500, CPU_F_I586,       0, "Pentium" }, //stepping A
          {  0x0510, CPU_F_I586,       0, "Pentium" },
          {  0x0520, CPU_F_I586,       0, "Pentium" },
          {  0x0530, CPU_F_I586,       0, "Pentium Overdrive" },
          {  0x0545, CPU_F_I586,       0, "Pentium (buggy-MMX)" }, /* MMX core crash - #2204 */
          {  0x0540, CPU_F_I586MMX,    0, "Pentium MMX" },
          {  0x0570, CPU_F_I586,       0, "Pentium" },
          {  0x0580, CPU_F_I586MMX,    0, "Pentium MMX" },
          {  0x0600, CPU_F_I686,       8, "Pentium Pro A-step" },
          {  0x0610, CPU_F_I686,       8, "Pentium Pro" },
          {  0x0630, CPU_F_I686MMX,    2, "Pentium II" }, /* Klamath (0.35um/512KB) */
          /* the following CPUs may have brand bits generated by x86ident */
          /* PII:  650: Deschutes A80522 (0.28um) */
          /* PII:  650: Tonga [Mobile] (0.25um) */
          /* Cely: 650: Covington (no On-Die L2 Cache) */
          {  0x0650, CPU_F_I686MMX,    2, "Pentium II" },
          {  0x1650, CPU_F_I686MMX,    2, "Celeron (Covington)" },
          {  0x4650, CPU_F_I686MMX,    2, "Pentium II Xeon" },
          /* Cely: 660: Dixon [Mobile] (128 [Cely] or 256 [PII] KB On-Die) */
          /* Cely: 660: Mendocino A80523 (0.25um, 128 KB On-Die L2 Cache) */
          {  0x0660, CPU_F_I686MMX,    2, "Pentium II" },
          {  0x1660, CPU_F_I686MMX,    2, "Celeron-A (Mendocino/Dixon)" },
        //{  0x0670, CPU_F_I686MMX,    2, "Pentium III" }, /* Katmai (0.25um/0.18um), 512KB, KNI */
          {  0x0670, CPU_F_I686MMX,    2, "Pentium III (Katmai)" },
/* !!! */ {  0x0670, CPU_F_I686MMX,    2, "Pentium III Xeon" },
          /* the following CPUs have brand bits from Intel */
        //{  0x0680, CPU_F_I686MMX,    2, "Pentium III" },
          {  0x1680, CPU_F_I686MMX,    2, "Celeron" },
          {  0x2680, CPU_F_I686MMX,    2, "Pentium III" },
/* !!! */ {  0x2680, CPU_F_I686MMX,    2, "Mobile Pentium III" },
          {  0x3680, CPU_F_I686MMX,    2, "Pentium III Xeon" },
          {  0x0690, CPU_F_I686MMX,    2, "Pentium III (Timna)" }, /* Timna:6547:0692 */
        //{  0x06A0, CPU_F_I686MMX,    2, "Pentium III" }, //0.18 um w/ 1/2MB on-die L2
          {  0x36A0, CPU_F_I686MMX,    2, "Pentium III Xeon" },
        //{  0x06B0, CPU_F_I686MMX,    2, "Pentium III" }, /* Tualatin:6547:46B1 */
          {  0x26B0, CPU_F_I686MMX,    2, "Pentium III (Tualatin)" },
          {  0x36B0, CPU_F_I686MMX,    2, "Celeron (Tualatin)" },
          {  0x46B0, CPU_F_I686MMX,    2, "Pentium III (Tualatin)" },
          {  0x66B0, CPU_F_I686MMX,    2, "Mobile Pentium III-M" },
          {  0x0700, CPU_F_I686MMX,    5, "Itanium" }, /* 6547:0701. #5 == RG RISC-rotate II */
        //{  0x0F00, CPU_F_I686MMX, 0x0B, "Pentium 4" }, /* 1.3 - 1.5GHz P4  (0.18u) */
        //{  0x0F10, CPU_F_I686MMX, 0x0B, "Pentium 4" }, /* 1.4 - 2.0GHz P4  (0.18u) */
        //{  0x0F20, CPU_F_I686MMX, 0x0B, "Pentium 4" }, /* >=2.0GHz P4-512k (0.13u) */
          {  0x8F00, CPU_F_I686MMX, 0x0B, "Pentium 4" },
          {  0xEF00, CPU_F_I686MMX, 0x0B, "Xeon" },
          {  0x8F10, CPU_F_I686MMX, 0x0B, "Pentium 4" },
          {  0xAF10, CPU_F_I686MMX, 0x0B, "Celeron 4" },
          {  0xBF10, CPU_F_I686MMX, 0x0B, "Xeon MP" },
          {  0xEF10, CPU_F_I686MMX, 0x0B, "Xeon" },
          {  0x9F20, CPU_F_I686MMX, 0x0B, "Pentium 4 (Northwood)" },
          {  0xBF20, CPU_F_I686MMX, 0x0B, "Xeon" },
          {  0xEF20, CPU_F_I686MMX, 0x0B, "Mobile Pentium 4-M" },
          {  0xFF20, CPU_F_I686MMX, 0x0B, "Mobile Celeron 4" },
          {  0x0000, 0,               -1, NULL }
          }; internalxref = &intelxref[0];
      vendorname = "Intel"; 

      cpuidbmask = 0xfff0; // brand/family/model/-, strip stepping bits
      if (cpuid == 0x0545) /* buggy mmx */
        cpuidbmask = 0xffff; // don't strip stepping bits
    }
    if (internalxref != NULL) /* we know about this vendor */
    {
      unsigned int pos;
      int maskedid = ( cpuid & cpuidbmask );
  
      for (pos = 0; internalxref[pos].cpuname; pos++ )
      {
        if (maskedid == internalxref[pos].cpuid) /* found it */
        {
          simpleid     = internalxref[pos].simpleid;
          featureflags = internalxref[pos].cpufeatures;
          detectedtype = dettype;
          // HACK!!!
          if ((featureflags & CPU_F_MMX) == CPU_F_MMX)
            simpleid |= 0x100;
          // KCAH !!!
          if ( internalxref[pos].cpuname )
          {
            strcpy( namebuf, vendorname );
            if (namebuf[0])
              strcat( namebuf, " ");
            if (chipname_override)
              strcat( namebuf, chipname_override );
            else
              strcat( namebuf, internalxref[pos].cpuname );
            detectedname = (const char *)&namebuf[0];
          }
          break;
        }
      }
    }
  }

  if (cpuname)
    *cpuname = detectedname;
  if (whattoret == 'c')
    return ((long)simpleid);
  else if (whattoret == 'f')
    return ((long)featureflags);
  return detectedtype;
}
#endif /* X86 */

/* ---------------------------------------------------------------------- */

#if (CLIENT_OS == OS_RISCOS)
#include <setjmp.h>
static jmp_buf ARMident_jmpbuf;
static void ARMident_catcher(int)
{
  longjmp(ARMident_jmpbuf, 1);
}
#endif

#if (CLIENT_CPU == CPU_ARM)
static long __GetRawProcessorID(const char **cpuname )
{
  static long detectedtype = -2L; /* -1 == failed, -2 == not supported */
  static const char *detectedname = NULL;
  static char namebuf[60];

  #if (CLIENT_OS == OS_RISCOS)
  if ( detectedtype == -2L )
  {
    /*
       ARMident() will throw SIGILL on an ARM 2 or ARM 250, because they 
       don't have the system control coprocessor. (We ignore the ARM 1
       because I'm not aware of any existing C++ compiler that targets it...)
    */
    signal(SIGILL, ARMident_catcher);
    if (setjmp(ARMident_jmpbuf))
    {
      /* we can't differentiate between ARM 2 and 250 */
      detectedtype = 0x2000;  /* fake up value from ARMident() */
      detectedname = "ARM 2 or 250"; /* set the name here too */
    }
    else
      detectedtype = ARMident();
    signal(SIGILL, SIG_DFL);

    detectedtype = (detectedtype >> 4) & 0xfff; // extract part number field
    if ((detectedtype & 0xf00) == 0) //old-style ID (ARM 3 or prototype ARM 600)
      detectedtype <<= 4;            // - shift it into the new form
    if (detectedtype == 0x710)
    {
      // the ARM 7500 returns an ARM 710 ID - need to look at its
      // integral IOMD unit to spot the difference
      u32 detectediomd = IOMDident();
      detectediomd &= 0xff00; // just want most significant byte
      if (detectediomd == 0x5b00)
        detectedtype = 0x7500;
      else if (detectediomd == 0xaa00)
        detectedtype = 0x7500FEL;
    }
  }
  #elif (CLIENT_OS == OS_LINUX)
  if (detectedtype == -2)
  {
    unsigned int n;
    FILE *cpuinfo;
    detectedtype = -1L;
    namebuf[0] = '\0';
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      n = 0;
      while(fgets(buffer, sizeof(buffer), cpuinfo))
      {
        if(memcmp(buffer, "Type\t\t: ", 8) == 0)
          n = 8;
        if(memcmp(buffer, "Processor\t: ", 12) == 0)
          n = 12;

        if(n != 0)
        {
          if (memcmp(&buffer[n], "Intel ", 6) == 0)
            n += 6;
          strncpy(namebuf, &buffer[n], sizeof(namebuf)-1);
          namebuf[sizeof(namebuf)-1] = '\0';
          break;
        }
      }
      fclose(cpuinfo);
    }

    if (namebuf[0])
    {
      static struct { const char *sig;  int rid; } sigs[] ={
                    { "unknown",    0x000}, /* <= from /proc/cpuinfo */
                    { "arm2",       0x200},
                    { "arm250",     0x250},
                    { "arm3",       0x300},
                    { "arm6",       0x600},
                    { "arm610",     0x610},
                    { "arm7",       0x700},
                    { "arm710",     0x710},
                    { "sa110",      0xA10},
                    { "ARM/VLSI ARM 6",         0x600},
                    { "ARM/VLSI ARM 610",       0x610},
                    { "ARM/VLSI ARM 7",         0x700},
                    { "ARM/VLSI ARM 710",       0x710},
                    { "StrongARM-110",          0xA10},
                    { "StrongARM-1100",         0xA11},
                    { "StrongARM-1110",         0xB11}
                    };
      /* assume unknown ID and unrecognized name in namebuf */
      detectedtype = 0;
      detectedname = ((const char *)&(namebuf[0]));

      for ( n = 0; n < (sizeof(sigs)/sizeof(sigs[0])); n++ )
      {
        int l = strlen(sigs[n].sig);

        if ((strncmp(namebuf, sigs[n].sig, l) == 0) &&
            ((namebuf[l] == '\0') || (namebuf[l] == ' ')))
        {
          /* known ID and recognized name */
          detectedtype = (long)sigs[n].rid;
          detectedname = NULL; /* fall through and use standard name */
          break;
        }
      }
    }
  }
  #elif (CLIENT_OS == OS_NETBSD)
  if (detectedtype == -2)
  {
    char buffer[256]; int mib[2];
    size_t len = (size_t)(sizeof(buffer)-1);
    mib[0]=CTL_HW; mib[1]=HW_MODEL;
    detectedtype = -1;

    if (sysctl(mib, 2, &buffer[0], &len, NULL, 0 ) != 0)
    {
      buffer[0] = '\0';
    }
    else if (len > 0)
    {
      buffer[len] = '\0';
      len = 0;
      while (buffer[len]=='-' || isalpha(buffer[len]) || isdigit(buffer[len]))
        len++;
      buffer[len] = '\0';
    }
    if (buffer[0]) /* sysctl() succeeded and name if clean */
    {
      static struct
        { const char *sig;  int rid; } sigs[] ={
        { "ARM2",       0X200},
        { "ARM250",     0X250},
        { "ARM3",       0X300},
        { "ARM6",       0X600},
        { "ARM610",     0X610},
        { "ARM7",       0X700},
        { "ARM710",     0X710},
        { "SA-110",     0XA10}
        };
      unsigned int n;
      detectedtype = 0;
      for ( n = 0; n < (sizeof(sigs)/sizeof(sigs[0])); n++ )
      {
        if (strcmp(buffer,sigs[n].sig)==0)
        {
          detectedtype = (long)sigs[n].rid;
          break;
        }
      }
      if (detectedtype == 0) /* unrecognized name */
      {
        strncpy( namebuf, buffer, sizeof(namebuf) );
        namebuf[sizeof(namebuf)-1] = '\0';
        detectedname = ((const char *)&(namebuf[0]));
      }
    } /* if (len > 0) */
  } /* if (detectedtype == -2) */
  #endif

  if (detectedtype > 0 && detectedname == NULL)
  {
    detectedname = ((const char *)&(namebuf[0]));
    switch (detectedtype)
    {
      case 0x200: strcpy( namebuf, "ARM 2" ); break;
      case 0x300: strcpy( namebuf, "ARM 3" ); break;
      case 0xA10: strcpy( namebuf, "StrongARM 110" ); break;
      case 0xA11: strcpy( namebuf, "StrongARM 1100" ); break;
      case 0xB11: strcpy( namebuf, "StrongARM 1110" ); break;
      case 0x250:
      case 0x600:
      case 0x610:
      case 0x700:
      case 0x7500:
      case 0x7500FEL:
      case 0x710:
      case 0x810: sprintf( namebuf, "ARM %lX", detectedtype );
                  break;
      default:    sprintf( namebuf, "%lX", detectedtype );
                  detectedtype = 0;
                  break;
    }
  }

  if ( cpuname )
    *cpuname = detectedname;
  return detectedtype;
}
#endif

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_MIPS)
static long __GetRawProcessorID(const char **cpuname)
{
  static const int ridPS2 = 99;      /* Please set a same rid of R5900 */

  static int detectedtype = -2L; /* -1 == failed, -2 == not supported */
  static const char *detectedname = NULL;
  static char namebuf[30];
  static struct { const char *name; int rid; } cpuridtable[] = {
                { "R2000"         ,       1  },
                { "R3000"         ,       2  },
                { "R3000A"        ,       3  },
                { "R3041"         ,       4  },
                { "R3051"         ,       5  },
                { "R3052"         ,       6  },
                { "R3081"         ,       7  },
                { "R3081E"        ,       8  },
                { "R4000PC"       ,       9  },
                { "R4000SC"       ,      10  },
                { "R4000MC"       ,      11  },
                { "R4200"         ,      12  },
                { "R4400PC"       ,      13  },
                { "R4400SC"       ,      14  },
                { "R4400MC"       ,      15  },
                { "R4600"         ,      16  },
                { "R5900"         ,      99  },   /* ridPS2 */
                { "R6000"         ,      17  },
                { "R6000A"        ,      18  },
                { "R8000"         ,      19  },
                { "R10000"        ,      20  }
                };
  
  #if (CLIENT_OS == OS_LINUX) || (CLIENT_OS == OS_PS2LINUX)

  /*  CPU detect algorithm was changed:  2002-05-31 by jt@distributed.net  /
  /   to suport R5900MM variants(PlayStation 2)                            /
  /   SCPH-10000: R5900 V1.4   laters: R5900 V2.0                          /
  /   Then I changed to detect space or null code.                        */

  if (detectedtype == -2L)
  {
    FILE *cpuinfo;
    detectedtype = -1L;
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      while(fgets(buffer, sizeof(buffer), cpuinfo)) 
      {
        const char *p = "cpu model\t\t: ";
        unsigned int n = strlen( p );
        if ( memcmp( buffer, p, n ) == 0 )
        {
          p = &buffer[n]; buffer[sizeof(buffer)-1]='\0';
          for ( n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
          {
            unsigned int l = strlen( cpuridtable[n].name );
            if ((!p[l] || isspace(p[l])) && memcmp( p, cpuridtable[n].name, l)==0)
            {
              detectedtype = (long)cpuridtable[n].rid;
              break;
            }
          }
          if (detectedtype == -1L)
          {
            for ( n = 0; *p && *p!='\r' && *p!='\n' && n<(sizeof(namebuf)-1); n++ ) 
              namebuf[n] = *p++;
            namebuf[n] = '\0';
            detectedname = (const char *)&namebuf[0];
            detectedtype = 0;
          }
          break;
        }
      }
      fclose(cpuinfo);
    }
  }
  #endif

  if (detectedtype > 0 && detectedname == NULL)
  {
    unsigned int n;
    detectedname = "";
    for ( n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
    {
      if (detectedtype == cpuridtable[n].rid )
      {
        if (detectedtype == ridPS2)
          strcpy( namebuf, "MIPS R5900MM(PS2 Emotion Engine)" );
        else
        {
          strcpy( namebuf, "MIPS " );
          strcat( namebuf, cpuridtable[n].name );
        }
        detectedname = (const char *)&namebuf[0];
        break;
      }
    }
  }
  if (cpuname)
    *cpuname = detectedname;
  return detectedtype;
}
#endif

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_SPARC)
static long __GetRawProcessorID(const char **cpuname)
{
  /* detectedtype reference is (0x100 + ((get_psr() >> 24) & 0xff)) */
  static long detectedtype = -2L; /* -1 == failed, -2 == not supported */
  static const char *detectedname = NULL;
  
  /* from linux kernel - last synced with kernel 2.4.19 */
  static struct { int psr_impl, psr_vers; const char *name; } cpuridtable[] = {
  /* from linux/arch/sparc/kernel/cpu.c */
  /* Sun4/100, 4/200, SLC */
  { 0, 0, "Fujitsu  MB86900/1A or LSI L64831 SparcKIT-40"},
  /* borned STP1012PGA */
  { 0, 4, "Fujitsu  MB86904"},
  { 0, 5, "Fujitsu TurboSparc MB86907"},
  /* SparcStation2, SparcServer 490 & 690 */
  { 1, 0, "LSI Logic Corporation - L64811"},
  /* SparcStation2 */
  { 1, 1, "Cypress/ROSS CY7C601"},
  /* Embedded controller */
  { 1, 3, "Cypress/ROSS CY7C611"},
  /* Ross Technologies HyperSparc */
  { 1, 0xf, "ROSS HyperSparc RT620"},
  { 1, 0xe, "ROSS HyperSparc RT625 or RT626"},
  /* ECL Implementation, CRAY S-MP Supercomputer... AIEEE! */
  /* Someone please write the code to support this beast! ;) */
  { 2, 0, "Bipolar Integrated Technology - B5010"},
  { 3, 0, "LSI Logic Corporation - unknown-type"},
  { 4, 0, "Texas Instruments, Inc. - SuperSparc-(II)"},
  /* SparcClassic  --  borned STP1010TAB-50*/
  { 4, 1, "Texas Instruments, Inc. - MicroSparc"},
  { 4, 2, "Texas Instruments, Inc. - MicroSparc II"},
  { 4, 3, "Texas Instruments, Inc. - SuperSparc 51"},
  { 4, 4, "Texas Instruments, Inc. - SuperSparc 61"},
  { 4, 5, "Texas Instruments, Inc. - unknown"},
  { 5, 0, "Matsushita - MN10501"},
  { 6, 0, "Philips Corporation - unknown"},
  { 7, 0, "Harvest VLSI Design Center, Inc. - unknown"},
  /* Gallium arsenide 200MHz, BOOOOGOOOOMIPS!!! */
  { 8, 0, "Systems and Processes Engineering Corporation (SPEC)"},
  { 9, 0, "Fujitsu or Weitek Power-UP"},
  { 9, 1, "Fujitsu or Weitek Power-UP"},
  { 9, 2, "Fujitsu or Weitek Power-UP"},
  { 9, 3, "Fujitsu or Weitek Power-UP"},
  /* from linux/arch/sparc64/kernel/cpu.c */
  { 0x17, 0x10, "TI UltraSparc I   (SpitFire)"},
  { 0x22, 0x10, "TI UltraSparc II  (BlackBird)"},
  { 0x17, 0x11, "TI UltraSparc II  (BlackBird)"},
  { 0x17, 0x12, "TI UltraSparc IIi"},
  { 0x17, 0x13, "TI UltraSparc IIe"},
  { 0x3e, 0x14, "TI UltraSparc III (Cheetah)"},
  /* old speling from earlier kernel versions */
  { 1, 0xe, "ROSS HyperSparc RT625"},
  { 9, 0, "Fujitsu #3"},
  { 4, 0, "Texas Instruments, Inc. - SuperSparc 50"},
  };

  #if (CLIENT_OS == OS_LINUX)
  if (detectedtype == -2L)
  {
    FILE *cpuinfo;
    detectedtype = -1L;
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      while(fgets(buffer, sizeof(buffer), cpuinfo)) 
      {
        const char *p = "cpu\t\t: ";
        unsigned int n = strlen( p );
        if ( memcmp( buffer, p, n ) == 0 )
        {
          static char namebuf[128];
          p = &buffer[n]; buffer[sizeof(buffer)-1]='\0';
          for ( n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
          {
            unsigned int l = strlen( cpuridtable[n].name );
            if ((!p[l] || isspace(p[l])) && memcmp( p, cpuridtable[n].name, l)==0)
            {
              detectedtype = 0x100 | ((cpuridtable[n].psr_impl & 0x0f)<<4) |
                             (cpuridtable[n].psr_vers & 0x0f);
              detectedname = cpuridtable[n].name;
              break;
            }
          }
          if (detectedtype == -1) /* found name but no ID */
          {
            for ( n = 0; *p && *p!='\r' && *p!='\n' && n<(sizeof(namebuf)-1); n++ )
              namebuf[n] = *p++;
            namebuf[n] = '\0';
            detectedname = (const char *)&namebuf[0];
            detectedtype = 0;
          }
          break;
        }
      }
      fclose(cpuinfo);
    }
  }
  #endif
  
  if (detectedtype >= 0x100 && detectedname == NULL)
  {
    int psr_impl = ((detectedtype>>4)&0xf);
    int psr_vers = ((detectedtype   )&0xf);
    unsigned int n;
    for (n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
    {
      if (( psr_impl == cpuridtable[n].psr_impl ) &&
          ( psr_vers == cpuridtable[n].psr_vers ))
      {
        detectedname = cpuridtable[n].name;
        break;
      }
    }
  }   
  if (cpuname)
    *cpuname = detectedname;
  return detectedtype;
}
#endif

/* ---------------------------------------------------------------------- */

#if (CLIENT_CPU == CPU_ALPHA)
static long __GetRawProcessorID(const char **cpuname)
{
  static char namebuf[30];
  static int detectedtype = -2L; /* -1 == failed, -2 == not supported */
  static const char *detectedname = NULL;
  static struct { int rid; const char *name;     } cpuridtable[] = {
                {       1, "EV3"                 },
                {       2, "EV4 (21064)"         },
                {       4, "LCA4 (21066/21068)"  },
                {       5, "EV5 (21164)"         },
                {       6, "EV45 (21064)"        },
                {       7, "EV56 (21164A)"       },
                {       8, "EV6 (21264)",        },
                {       9, "EV56 (21164PC)"      },
                {      10, "EV57"                },
                {      11, "EV67"                },
                {      12, "EV68CB"              },
                {      13, "EV68AL"              },
                {      14, "EV68CX"              },
                {      15, "EV69"                },
                {      16, "EV7"                 },
                {      17, "EV79"                }
                };
  
  #if (CLIENT_OS == OS_DEC_UNIX)
  if (detectedtype == -2)
  {
    long buf;
    int st = 0;
    detectedtype = -1;
    if (getsysinfo(GSI_PROC_TYPE,(char *)&buf,sizeof(buf),st,NULL,NULL)!=-1)
      detectedtype = (buf & 0xffff);
    if (detectedtype <= 0)
      detectedtype = -1;
  }
  #elif (CLIENT_OS == OS_WIN32)
  if (detectedtype == -2L)
  {
    SYSTEM_INFO si;
    si.wProcessorLevel = si.wProcessorArchitecture = 0;
    detectedtype = -1L;
    GetSystemInfo( &si );
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA &&
        si.wProcessorLevel != 0)
    {
      unsigned int ref;
      for (ref = 0; (ref < sizeof(cpuridtable)/sizeof(cpuridtable[0])); ref++)
      {
        char *q = strchr(cpuridtable[ref].name,'(');
        if (q) 
        {
          if (si.wProcessorLevel == atoi(q+1))
          {
            detectedtype = cpuridtable[ref].rid;
            break;
          }
        }  
      }
      if (detectedtype == -1)
      {
        sprintf( namebuf, "%u", si.wProcessorLevel );
        detectedname = namebuf;
        detectedtype = 0;
      }
    }    
  }
  #elif (CLIENT_OS == OS_LINUX)
  if (detectedtype == -2L)
  {
    FILE *cpuinfo;
    detectedtype = -1L;
    if ((cpuinfo = fopen( "/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[256];
      while(fgets(buffer, sizeof(buffer), cpuinfo)) 
      {
        const char *p = "cpu model\t\t: ";
        unsigned int n = strlen( p );
        if ( memcmp( buffer, p, n ) == 0 )
        {
          static struct 
           { const char *sig;  int rid; } sigs[] = {
           { "EV3",            1      },
           { "EV4",            2      },
           { "Unknown 1",      3      }, /* 2.0.x kernel */
           { "Unknown",        3      }, /* 2.2.x kernel */
           { "LCA4",           4      },
           { "EV5",            5      },
           { "EV45",           6      },
           { "EV56",           7      },
           { "EV6",            8      },
           { "PCA56",          9      },
           { "PCA57",         10      }, /* 2.2.x kernel */
           { "EV67",          11      },
           { "EV68CB",        12      },
           { "EV68AL",        13      },
           { "EV68CX",        14      },
           { "EV69",          15      },
           { "EV7",           16      },
           { "EV79",          17      }

           };
          p = &buffer[n]; buffer[sizeof(buffer)-1]='\0';
          for ( n = 0; n < (sizeof(sigs)/sizeof(sigs[0])); n++ )
          {
            unsigned int l = strlen( sigs[n].sig );
            if ((!p[l] || isspace(p[l])) && memcmp( p, sigs[n].sig, l)==0)
            {
              detectedtype = (long)sigs[n].rid;
              break;
            }
          }
          if (detectedtype == -1L)
          {
            for ( n = 0; *p && *p!='\r' && *p!='\n' && n<(sizeof(namebuf)-1); n++ )  
              namebuf[n] = *p++;
            namebuf[n] = '\0';
            detectedname = (const char *)&namebuf[0];
            detectedtype = 0;
          }
          break;
        }
      }
      fclose(cpuinfo);
    }
  }
  #endif
  
  if (detectedtype > 0 && detectedname == NULL)
  {
    unsigned int n;
    detectedname = "";
    for ( n = 0; n < (sizeof(cpuridtable)/sizeof(cpuridtable[0])); n++ )
    {
      if (detectedtype == cpuridtable[n].rid )
      {
        strcpy( namebuf, "Alpha " );
        strcat( namebuf, cpuridtable[n].name );
        detectedname = (const char *)&namebuf[0];
        break;
      }
    }
  }
  
  if (cpuname)
    *cpuname = detectedname;
  return detectedtype;
}
#endif

/* ---------------------------------------------------------------------- */

//get (simplified) cpu ident by hardware detection
long GetProcessorType(int quietly)
{
  // only successful detection / detection of a new unknown cpu type gets logged to file
  long retval = -1L;
  const char *apd = "Automatic processor type detection ";
  #if (CLIENT_CPU == CPU_ALPHA)   || (CLIENT_CPU == CPU_68K) || \
      (CLIENT_CPU == CPU_POWERPC) || (CLIENT_CPU == CPU_X86) || \
      (CLIENT_CPU == CPU_ARM)     || (CLIENT_CPU == CPU_MIPS) || \
      (CLIENT_CPU == CPU_SPARC)
  {
    const char *cpuname = NULL;
    long rawid = __GetRawProcessorID(&cpuname);
    if (rawid < 0)
    {
      retval = -1L;  
      if (!quietly)
        LogScreen("%s%s.\n", apd, ((rawid == -1L)?("failed"):("is not supported")));
    }
    else if (rawid == 0)
    {
      retval = -1L;  
      if (!quietly)
        Log("%sdid not\nrecognize the processor (tag: \"%s\")\n", apd, (cpuname?cpuname:"???") );
    }
    else 
    {
      if (!quietly)
      {
        if (cpuname == NULL || *cpuname == '\0')
          Log("%sdid not\nrecognize the processor (id: %ld)\n", apd, rawid );
        else
          Log("%sfound\na%s %s processor.\n",apd, 
             ((strchr("aeiou8", tolower(*cpuname)))?("n"):("")), cpuname);
      }
      retval = rawid; /* let selcore figure things out */
      #if (CLIENT_CPU == CPU_X86) /* simply too many core<->cpu combinations */
      if ((retval = __GetRawProcessorID(NULL,'c')) < 0)
        retval = -1;
      #endif
    }
  }  
  #else
  {
    if (!quietly)
      LogScreen("%sis not supported.\n", apd );
  }
  #endif
  return retval;
}  

#if 0
//get cpu ident by hardware detection
long GetProcessorID()
{
  long retval = -1L;
  #if (CLIENT_CPU == CPU_ALPHA)   || (CLIENT_CPU == CPU_68K) || \
      (CLIENT_CPU == CPU_POWERPC) || (CLIENT_CPU == CPU_X86) || \
      (CLIENT_CPU == CPU_ARM)     || (CLIENT_CPU == CPU_MIPS) || \
      (CLIENT_CPU == CPU_SPARC)
  {
    long rawid = __GetRawProcessorID(NULL);
    if (rawid < 0)
    {
      retval = -1L;  
    }
    else if (rawid == 0)
    {
      retval = -1L;  
    }
    else 
    {
      retval = rawid;
    }
  }  
  #endif
  return retval;
}
#endif

//get a set of supported processor features
//cores may get disabled due to missing features
unsigned long GetProcessorFeatureFlags()
{
  #if (CLIENT_CPU == CPU_X86)
  return __GetRawProcessorID(NULL, 'f');
  #else
  return 0;
  #endif
}

/* ---------------------------------------------------------------------- */

// Originally intended as tool to assist in managing the processor ID table
// for x86, I now (after writing it) realize that it could also get users on
// non-x86 platforms to send us *their* processor detection code. :) - Cyrus

void GetProcessorInformationStrings( const char ** scpuid, const char ** smaxscpus, const char ** sfoundcpus )
{
  const char *maxcpu_s, *foundcpu_s, *cpuid_s;

#if (CLIENT_CPU == CPU_ALPHA)   || (CLIENT_CPU == CPU_68K) || \
    (CLIENT_CPU == CPU_POWERPC) || (CLIENT_CPU == CPU_X86) || \
    (CLIENT_CPU == CPU_ARM)     || (CLIENT_CPU == CPU_MIPS) || \
    (CLIENT_CPU == CPU_SPARC)      
  long rawid = __GetRawProcessorID(&cpuid_s);
  if (rawid < 0)
    cpuid_s = ((rawid==-1)?("?\n\t(identification failed)"):
              ("none\n\t(client does not support identification)"));
  else
  {
    static char namebuf[60];
    if (cpuid_s == NULL) cpuid_s = "*unknown*";
    if (*cpuid_s =='\0') cpuid_s = "???";
  #if (CLIENT_CPU == CPU_ARM)
    namebuf[0] = '\0';
    if (rawid != 0) /* if rawid == 0, then cpuid_s == "%lX" */
      sprintf( namebuf, "%lX\n\tname: ", rawid );
    strcat( namebuf, cpuid_s ); /* always valid */
    #if (CLIENT_OS == OS_RISCOS && defined(HAVE_X86_CARD_SUPPORT))
    if (riscos_count_cpus() == 2)
      strcat(strcat(namebuf,"\n\t+ "),riscos_x86_determine_name());
    #endif
  #elif (CLIENT_CPU == CPU_X86)
    namebuf[0] = '\0';
    if (rawid != 0) /* if rawid == 0, then cpuid_s == "%04x:%04x" */
      sprintf( namebuf, "%04X:%04X\n\tname: ",(int)((rawid>>16)&0xffff),(int)(rawid&0xffff));
    strcat( namebuf, cpuid_s ); /* always valid */
  #else
    sprintf(namebuf, "%ld\n\tname: %s\n", rawid, cpuid_s );
  #endif
    cpuid_s = ((const char *)(&namebuf[0]));
  }    
#else    
  cpuid_s = "none\n\t(client does not support identification)";
#endif

  #if defined(CLIENT_SUPPORTS_SMP)
  {
    maxcpu_s = "128"; /* just some arbitrary number */
    #if (CLIENT_OS == OS_RISCOS) && defined(HAVE_X86_CARD_SUPPORT)
    if (GetNumberOfDetectedProcessors() > 1)
      maxcpu_s = 2; /* thread 0 is ARM, thread 1 is x86 */
    #endif
  }
  #elif (!defined(CORES_SUPPORT_SMP))
    maxcpu_s = "1\n\t(cores are not thread-safe)";
  #elif (CLIENT_OS == OS_RISCOS)
    #if defined(HAVE_X86_CARD_SUPPORT)
    maxcpu_s = "2\n\t(with RiscPC x86 card)";
    #else
    maxcpu_s = "1\n\t(client-build does not support multiple processors)";
    #endif
  #else
    maxcpu_s = "1\n\t(OS or client-build does not support multiple processors)";
  #endif

  int cpucount = GetNumberOfDetectedProcessors();
  if (cpucount < 1)
    foundcpu_s = "1\n\t(OS does not support detection)";
  else
  {
    static char foundcpu_b[6];
    sprintf( foundcpu_b, "%d", cpucount );
    foundcpu_s = ((const char *)(&foundcpu_b[0]));
  }

  if ( scpuid ) *scpuid = cpuid_s;
  if ( smaxscpus ) *smaxscpus = maxcpu_s;
  if ( sfoundcpus ) *sfoundcpus = foundcpu_s;
  return;
}

/* ---------------------------------------------------------------------- */

void DisplayProcessorInformation(void)
{
  const char *scpuid, *smaxscpus, *sfoundcpus;
  GetProcessorInformationStrings( &scpuid, &smaxscpus, &sfoundcpus );

  LogRaw("Automatic processor identification tag: %s\n"
    "Number of processors detected by this client: %s\n"
    "Number of processors supported by this client: %s\n",
    scpuid, sfoundcpus, smaxscpus );
  return;
}

/* ---------------------------------------------------------------------- */
