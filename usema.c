/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Unified Semaphores System (first name which comes to my mind ;)
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
 * $Id: usema.c,v 0.1 2007/04/03 08:26:44 diegocr Exp $
 * 
 * ***** END LICENSE BLOCK ***** */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include "palloc.h"
#include "debug.h"

typedef struct uSemaphore
{
	ULONG MagicID;
	#define uSemagicID	0x0ddf74E5
	
	struct Task * manager;		// the task who set up the uSema system
	struct Task * Owner;		// the task who owns the "semaphore"
	
	struct MinList Waiters;		// tasks waiting the "semaphore"
	
	BOOL blocked;			// is the locking system blocked?
	
} uSemaphore;

typedef struct uSemaWaiter
{
	struct MinNode node;
	struct Task * waiter;
} uSemaWaiter;

STATIC uSemaphore * us = NULL;

#if !defined(IsMinListEmpty)
# define IsMinListEmpty(x)	(((x)->mlh_TailPred) == (struct MinNode *)(x))
#endif

//------------------------------------------------------------------------------

BOOL uSemaInit( VOID )
{
	if((us = Malloc(sizeof(uSemaphore))))
	{
		us->MagicID = uSemagicID;
		us->manager = FindTask(NULL);
		us->Owner   = NULL;
		us->blocked = FALSE;
		
		NewList((struct List *) &(us->Waiters));
	}
	
	return((us != NULL) ? TRUE : FALSE);
}

//------------------------------------------------------------------------------

VOID uSemaClear( VOID )
{
	ENTER();
	
	if((us != NULL) && (us->MagicID == uSemagicID))
	{
		uSemaWaiter * usw;
		
		// half of this may isn't needed, but i'm too paranoic ;-)
		Forbid();
		us->blocked = TRUE;
		
		// try to unlock the uSema if im the owner..
		if(us->manager == us->Owner)
			us->Owner = NULL;
		else // else break who owns it..
			Signal( us->Owner, SIGBREAKF_CTRL_C );
		
		// if there are waiting task, break them..
		while((usw = (uSemaWaiter *)RemHead((struct List *)&(us->Waiters))))
		{
			Signal( usw->waiter, SIGBREAKF_CTRL_C );
			Free(usw);
		}
		
		Permit();
		
		DBG_ASSERT(us->Owner == NULL);
		DBG_ASSERT(IsMinListEmpty(&(us->Waiters))==TRUE);
		
		/**
		 * Wait until the task who owns the uSema unlocks it..
		 * NOTE: this is some exceptional case, since us->Owner 
		 * SHOULD be NULL already...
		 */
		while(TRUE)
		{
			BOOL done;
			
			Forbid();
			done = (us->Owner == NULL);
			Permit();
			
			if(done) break;
			
			Delay(TICKS_PER_SECOND);
		}
		
		us->MagicID = 0xDEADBADF;
		Free(us);
		us = NULL;
	}
	
	LEAVE();
}

//------------------------------------------------------------------------------

BOOL uSemaLock( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	
	if((us != NULL) && (us->MagicID == uSemagicID) && !us->blocked)
	{
		BOOL lock = FALSE;
		uSemaWaiter * usw;
		struct Task * me = FindTask(NULL);
		
		Forbid();
		
		if(us->Owner == NULL)
		{
			DBG("Task $%lx (%s) own from now on the uSema!\n", me, me->tc_Node.ln_Name);
			
			us->Owner = me;
			rc = TRUE;
		}
		else if((usw = Malloc(sizeof(uSemaWaiter))))
		{
			DBG("Task $%lx (%s) is requesting the uSema!\n", me, me->tc_Node.ln_Name);
			
			usw->waiter = me;
			
			AddTail((struct List *)&(us->Waiters),(struct Node *)usw);
			lock = rc = TRUE;
		}
		
		Permit();
		
		if( lock == TRUE )
		{
			ULONG sigs;
			
			// clear the signal to avoid returning inmediately..
			SetSignal(NULL,SIGF_SINGLE);
			
			sigs = Wait( SIGF_SINGLE | SIGBREAKF_CTRL_C );
			
			if(sigs & SIGF_SINGLE)
			{
				/**
				 * If someone breaked me, prevent from
				 * continuing running (thats what the caller
				 * MUST do) as it is probably at exit step..
				 */
				
				rc = uSemaLock ( ) ;
			}
		}
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

VOID uSemaUnLock( VOID )
{
	ENTER();
	
	if((us != NULL) && (us->MagicID == uSemagicID))
	{
		struct Task * me = FindTask(NULL);
		
		Forbid();
		
		DBG_ASSERT(me == us->Owner);
		
		if(me == us->Owner)
		{
			DBG("Task $%lx (%s) is unlocking the uSema!\n", me, me->tc_Node.ln_Name);
			
			us->Owner = NULL;
			
			if(!IsMinListEmpty(&(us->Waiters)))
			{
				uSemaWaiter * usw;
				
				usw = (uSemaWaiter *)RemHead((struct List *)&(us->Waiters));
				DBG_ASSERT(usw != NULL);
				
				if(usw != NULL)
				{
					DBG("Leting know task $%lx (%s) that he can own the uSema!\n", usw->waiter, usw->waiter->tc_Node.ln_Name);
					
					Signal( usw->waiter, SIGF_SINGLE );
					Free(usw);
				}
			}
		}
		
		Permit();
	}
	
	LEAVE();
}
