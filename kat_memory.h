/**
 * kat_memory.h
 */

#ifndef __KAT_MEMORY_H
#define __KAT_MEMORY_H

#ifdef _WIN32
#  include <memoryapi.h>

#define KAT_VMEM(baseaddr, size) \
        VirtualAlloc(baseaddr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#else /* NOTE: If using Linux or MacOS */
#  include <sys/mman.h>
/**
 * NOTE: MAP_ANONYMOUS is not defined on some other
 *       UNIX systems, use MAP_ANON instead.
 */
#  ifndef MAP_ANONYMOUS
#  define MAP_ANONYMOUS MAP_ANON
#  endif
# define KAT_VMEM(baseaddr, size) \
        mmap(baseaddr,size,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
#endif

#endif /* __KAT_MEMORY_H */
