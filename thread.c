/*
 * athread.c - AmigaOS based thread system
 * Copyright (c) 2007, Diego Casorran <diegocr at users dot sf dot net>
 * All rights reserved.
 * 
 * This file contains helpers/wrappers for laziness people, like me ;-)
 */

#include "thread.h"
#include "debug.c"
#include "palloc.c"
#include "usema.c"
#include "athread.c"

ThreadManager *tm;

// Backward compatibility
typedef void (*ufcb)(void *);

STATIC VOID QuickThreadStub(Thread *t)
{
	ufcb user_func = t->args[0];
	
	user_func(t->args[1]);
	return TRUE;
}

APTR QuickThread(APTR func,APTR data)
{
	Thread *t;
	ULONG args[] = { func, data, TAG_DONE };
	
	t = ThreadLaunchA(tm, 0, 0, 0, QuickThreadStub, args);
	
	return((t != NULL) ? t->task : NULL);
}
