/*
 * athread.c - AmigaOS based thread system
 * Copyright (c) 2007, Diego Casorran <diegocr at users dot sf dot net>
 * All rights reserved.
 * 
 * This file contains helpers/wrappers for laziness people, like me ;-)
 */

#include "debug.h"
#include "palloc.h"
#include "usema.h"
#include "athread.h"

GLOBAL ThreadManager *tm;

extern __inline void aThreadExit()
{
	if( tm )
	{
		CleanupThreadManager(tm);
		uSemaClear();
		ClearMemoryPool();
		tm = NULL;
	}
}

extern __inline int aThreadInit()
{
	if(InitMemoryPool())
	{
		if(uSemaInit())
		{
			if((tm = InitThreadManager(0)))
			{
				return(1);
			}
			
			uSemaClear();
		}
		
		ClearMemoryPool();
	}
	
	return(0);
}

#define aThreadWatch(_Sigs) do { \
	if((_Sigs) & tm->sigmask ) \
		ThreadManagerDispatcher( tm ); \
} while(0)

// Backward compatibility
GLOBAL APTR QuickThread(APTR func,APTR data);
