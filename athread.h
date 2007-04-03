/*
 * athread.h - AmigaOS based thread system
 * Copyright (c) 2007, Diego Casorran <diegocr at users dot sf dot net>
 * All rights reserved.
 * 
 * Redistribution  and  use  in  source  and  binary  forms,  with  or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * -  Redistributions  of  source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this  list  of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * -  Neither  the name of the author(s) nor the names of its contributors may
 * be  used  to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND  ANY  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE  DISCLAIMED.   IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT OF
 * SUBSTITUTE  GOODS  OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION)  HOWEVER  CAUSED  AND  ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT,  STRICT  LIABILITY,  OR  TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * $Id: athread.h,v 0.1 2007/03/12 11:53:23 diegocr Exp $
 */

#ifndef ATHREAD_H
#define ATHREAD_H

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>

//------------------------------------------------------------------------------
/* use pragma on ppc systems to adjust structs to 68k fashion */

#if defined(__PPC__)
# if defined(__GNUC__)
#  pragma pack(2)
# elif defined(__VBCC__)
#  pragma amiga-align
# endif
#endif

//------------------------------------------------------------------------------

typedef struct ThreadManager
{
	ULONG MagicID;
	
	ULONG breaksig, sigmask;
	struct MsgPort *sigport;
	struct Task * task;
	struct MinList threads;
} ThreadManager;

typedef struct Thread
{
	struct MinNode node;
	ULONG MagicID;
	
	struct Task * task;
	struct MsgPort * port;
	struct Task * parent;
	struct MsgPort * parent_port;
	
	struct SignalSemaphore sem;
	APTR func, handler;
	ULONG *args;
	BOOL working, success;
	
	ThreadManager * manager;
} Thread;

typedef struct ThreadMessage
{
	struct Message msg;
	Thread *supervisor;
	UWORD flags;
	APTR udata;
	BOOL replyed;
} ThreadMessage;

//------------------------------------------------------------------------------
/* restore ppc pragma adjustment */

#if defined(__PPC__)
# if defined(__GNUC__)
#  pragma pack()
# elif defined(__VBCC__)
#  pragma default-align
# endif
#endif

//------------------------------------------------------------------------------

GLOBAL ThreadManager *InitThreadManager(ULONG breaksig);
GLOBAL VOID CleanupThreadManager(ThreadManager *m);

GLOBAL VOID ThreadManagerDispatcher(ThreadManager *m);
GLOBAL BOOL PutThreadMsg(Thread *t, struct MsgPort *from,
	struct MsgPort *to, UWORD flags, APTR udata);

GLOBAL Thread *ThreadLaunchA(ThreadManager *m,STRPTR name,
	BYTE pri,APTR handler,APTR func,ULONG *args);

#define ThreadLaunch(m,n,p,h,f,args...)		\
({						\
	ULONG _args[] = { args };		\
	ThreadLaunchA(m,n,p,h,f,_args)	;	\
})

#define qThread( tm, func ) ThreadLaunch(tm,0,0,0,func,TAG_DONE)

//------------------------------------------------------------------------------

#define TMANAGER_MAGICID	0xDFa30000
#define THREAD_MAGICID		0xDFa31111
#define TDELETED_MAGICID	0xDFa3DEAD

#define VALID_THREAD(t)		(t && t->MagicID == THREAD_MAGICID)
#define VALID_TMANAGER(tm)	(tm && tm->MagicID == TMANAGER_MAGICID)
#define ATHREAD_VALID(t)	(VALID_THREAD(t) && VALID_TMANAGER(t->manager))
#define THREAD_DELETED(t)	(!t || t->MagicID == TDELETED_MAGICID)

//------------------------------------------------------------------------------

#ifdef DEBUG
# define DIT( iuy, oou, ert ) \
	DBG("Invalid MagicID for %s 0x%08lx, must be 0x%08lx but it is"	\
		" 0x%08lx (%s)", iuy, oou, ert, oou->MagicID,		\
		THREAD_DELETED(oou) ? "it has been deleted":"corrupted")

# define DBG_THREAD(T)							\
	do {								\
		if( T != NULL )						\
		{							\
			if((T)->MagicID != THREAD_MAGICID)		\
			{						\
				DIT("thread",T,THREAD_MAGICID);		\
			}						\
									\
			if((T)->manager != NULL )			\
			{						\
				if(!VALID_TMANAGER((T)->manager))	\
				{					\
					DIT("manager",(T)->manager,TMANAGER_MAGICID); \
				}					\
			}						\
									\
			DBG("thread=$%lx task=$%lx parent=$%lx "	\
				"manager=$%lx func=%lx arg1=0x%lx\n",T,	\
				(T)->task,(T)->parent,(T)->manager,	\
				(T)->func,*((T)->args) );		\
		}							\
	} while(0)
#else /* DEBUG */
# define DBG_THREAD(t) ((void)0)
#endif /* DEBUG */

//------------------------------------------------------------------------------

#endif /* ATHREAD_H */
