/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2007, Diego Casorran <dcasorran@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 ** Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 ** Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */


#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"

struct SignalSemaphore gSemaphore;
APTR __private_memory_pool;

#define MEMORY_MAGICID	0xef1cb0a3
#define MEMORY_FREEDID	0xffff8520

BOOL InitMemoryPool( VOID )
{
	InitSemaphore(&gSemaphore);
	
	// instead using MEMF_CLEAR, use the provided Bzero implementation
	__private_memory_pool = CreatePool(MEMF_PUBLIC, 4096, 512);
	
	return((__private_memory_pool != NULL) ? TRUE : FALSE);
}

VOID ClearMemoryPool( VOID )
{
	if(__private_memory_pool != NULL)
		DeletePool( __private_memory_pool );
}

APTR Malloc( ULONG size )
{
	ULONG *mem;
	
	if(((long)size) <= 0)
		return NULL;
	
	ObtainSemaphore(&gSemaphore);
	
	size += sizeof(ULONG) * 2 + MEM_BLOCKMASK;
	size &= ~MEM_BLOCKMASK;
	
	if((mem = AllocPooled(__private_memory_pool, size)))
	{
		*mem++ = MEMORY_MAGICID;
		*mem++ = size;
	}
	ReleaseSemaphore(&gSemaphore);
	
	return mem;
}

VOID Free(APTR mem)
{
	ULONG size,*omem=mem;
	
	if( ! mem ) return;
	
	ObtainSemaphore(&gSemaphore);
	
	// get the allocation size
	size = *(--omem);
	
	// be sure we allocated this memory...
	if(*(--omem) == MEMORY_MAGICID)
	{
		*omem = MEMORY_FREEDID;
		FreePooled(__private_memory_pool, omem, size );
	}
	else {
		DBG("Memory area 0x%08lx cannot be freed! (%s)\n\a", mem,
		   (*omem == MEMORY_FREEDID) ? "already freed":"isn't mine");
	}
	
	ReleaseSemaphore(&gSemaphore);
}

APTR Realloc( APTR old_mem, ULONG new_size )
{
	APTR new_mem;
	ULONG old_size,*optr = old_mem;
	
	if(!old_mem) return Malloc( new_size );
	
	if(*(optr-2) != MEMORY_MAGICID)
	{
		DBG("Memory area 0x%08lx isn't known\n", old_mem);
		return NULL;
	}
	
	old_size = (*(optr-1)) - (sizeof(ULONG)*2);
	if(new_size <= old_size)
		return old_mem;
	
	if((new_mem = Malloc( new_size )))
	{
		CopyMem( old_mem, new_mem, old_size);
		Free( old_mem );
	}
	
	return new_mem;
}

VOID bZero( APTR data, ULONG size )
{
	register unsigned long * uptr = (unsigned long *) data;
	register unsigned char * sptr;
	
	// first blocks of 32 bits
	while(size >= sizeof(ULONG))
	{
		*uptr++ = 0;
		size -= sizeof(ULONG);
	}
	
	sptr = (unsigned char *) uptr;
	
	// now any pending bytes
	while(size-- > 0)
		*sptr++ = 0;
}

#ifdef TEST
int main()
{
	if(InitMemoryPool())
	{
		APTR data;
		
		if((data = Malloc(1025)))
		{
			Free(data);
			Free(data);
		}
		
		ClearMemoryPool();
	}
}
#endif
