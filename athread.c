/*
 * athread.c - AmigaOS based thread system
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
 * $Id: athread.c,v 0.1 2007/03/12 11:53:23 diegocr Exp $
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <dos/dostags.h>
#include "debug.h"
#include "palloc.h"
#include "athread.h"

#if !defined(IsMinListEmpty)
# define IsMinListEmpty(x)	(((x)->mlh_TailPred) == (struct MinNode *)(x))
#endif

// declaration for thread function calls
typedef BOOL (*tStub)(Thread *t);
// 
typedef VOID (*tHandler)(Thread *t,UWORD flags,APTR udata);

//------------------------------------------------------------------------------

ThreadManager *InitThreadManager(ULONG breaksig)
{
	ThreadManager *m;
	
	ENTER();
	
	if((m = Malloc(sizeof(*m))))
	{
		m->MagicID = TMANAGER_MAGICID;
		
		// port to communicate with threads
		if((m->sigport = CreateMsgPort ( )))
		{
			m->sigmask = (1L << (m->sigport->mp_SigBit));
			
			if((m->breaksig = breaksig) == 0 )
				m->breaksig = SIGBREAKF_CTRL_F;
			
			m->sigmask |= m->breaksig;
			
			NewList((struct List *) &m->threads);
			
			m->task = FindTask(NULL);
			
			DBG("Created new threads manager from task 0x%08lx\n", (ULONG)m->task);
		}
		else
		{
			Free( m );
			m = NULL;
		}
	}
	
	RETURN(m);
	return(m);
}

//------------------------------------------------------------------------------

VOID CleanupThreadManager(ThreadManager *m)
{
	ENTER();
	DBG_POINTER(m);
	
	if(VALID_TMANAGER(m))
	{
		struct MinNode * node;
		
		node = m->threads.mlh_Head;
		
		while( node->mln_Succ )
		{
			DBG_THREAD(((Thread *)node));
			Signal(((Thread *)node)->task, SIGBREAKF_CTRL_C );
			((Thread *)node)->handler = NULL;
			node = node->mln_Succ;
		}
		
		while(1)
		{
			ThreadManagerDispatcher(m);
			if(IsMinListEmpty(&m->threads)) break;
			Wait(m->sigmask | SIGBREAKF_CTRL_C );
		}
		
		DeleteMsgPort(m->sigport);
		m->MagicID = TDELETED_MAGICID;
		Free( m );
	}
	
	LEAVE();
}

//------------------------------------------------------------------------------

STATIC VOID ThreadStub( VOID )
{
	Thread *t;
	
	ENTER();
	
	Wait(SIGF_SINGLE);
	t = (Thread *)(FindTask(NULL)->tc_UserData);
	
	DBG_ASSERT(ATHREAD_VALID(t));
	DBG("Started new thread from parent 0x%08lx (%lx,%lx)\n",t->parent,t,t->task);
	ObtainSemaphore(&t->sem);
	
	Forbid ( );
	Signal ( t->parent, SIGF_SINGLE );
	Permit ( );
	
	if((t->port = CreateMsgPort()))
	{
		tStub stub = t->func;
		ThreadMessage * tmsg;
		
		ReleaseSemaphore(&t->sem);
		
		DBG_POINTER(stub);
		if( stub != NULL )
		{
			t->success = stub(t);
		}
		
		ObtainSemaphore(&t->sem);
		while((tmsg = (ThreadMessage *) GetMsg(t->port)))
		{
			if(!tmsg->replyed) {
				ReplyMsg((struct Message *)tmsg);
			}
		}
		
		DeleteMsgPort(t->port);
		t->port = NULL;
	}
	
	RETURN(t->success);
	
	Forbid();
	t->working = FALSE;
	ReleaseSemaphore(&t->sem);
	Signal(t->parent, t->manager->breaksig);
}

#ifdef __MORPHOS__
struct EmulLibEntry ThreadStub_GATE = {
	TRAP_LIBNR, 0, (void (*)(void)) &ThreadStub
};
#endif

//------------------------------------------------------------------------------

Thread *ThreadLaunchA(ThreadManager *m,STRPTR name,BYTE pri,APTR handler,APTR func,ULONG *args)
{
	Thread *t = NULL;
	
	ENTER();
	
	if( !m || (m->MagicID != TMANAGER_MAGICID))
	{
		DBG("Invalid manager\a\n");
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	}
	else if((t = (Thread *) Malloc(sizeof(*t))))
	{
		bZero( t, sizeof(*t));
		
		t->MagicID	= THREAD_MAGICID;
		t->manager	= m;		// manager this thread 
						// belongs to
		t->parent	= m->task;	// parent task
		t->parent_port	= m->sigport;	// parent signal port
		t->func		= func;		// to call in thread context
		t->handler	= handler;	// handler when communicating
						// with parent task (sigport)
		t->args		= args;		// user data
		t->working	= TRUE;		// set working from now on
		InitSemaphore(&t->sem);		// PRIVATE thread semaphore
		
		// make sure the thread has a name...
		if( name == NULL )
			name = m->task->tc_Node.ln_Name;
		
		// creates the thread
		t->task = (struct Task *)CreateNewProcTags(
			#ifdef __MORPHOS__
			NP_Entry,    (ULONG) &ThreadStub_GATE,
			#else
			NP_Entry,    (ULONG) ThreadStub,
			#endif
			NP_Name,     (ULONG) name,
			NP_Priority, (ULONG) pri,
			NP_StackSize, 65535,
			TAG_DONE
		);
		
		if( t->task )
		{
			AddTail((struct List *) &m->threads, (struct Node *)t);
			
			DBG("Created new thread at address $%lx (%lx)\n",t->task,t);
			
			t->task->tc_UserData = t;
			Signal(t->task, SIGF_SINGLE);
			Wait(SIGF_SINGLE);
		}
		else {
			Free(t); t = NULL;
			
			DBG("CreateNewProcTags() FAILED, ioerr = %ld\n", IoErr());
		}
	}
	
	RETURN(t);
	return(t);
}

//------------------------------------------------------------------------------

VOID ThreadManagerDispatcher(ThreadManager *m)
{
	ENTER();
	DBG_POINTER(m);
	
	if(VALID_TMANAGER(m))
	{
		struct MinNode * node, * succ;
		ThreadMessage * tmsg;
		Thread * t;
		
		node = m->threads.mlh_Head;
		
		while( node->mln_Succ )
		{
			t = (Thread *) node;
			succ = node->mln_Succ;
			
			DBG_THREAD(t);
			if(ATHREAD_VALID(t))
			{
				ObtainSemaphore(&t->sem);
				
				if(t->working == FALSE)
				{
					DBG("Thread %lx is no longer working, removing...\n",t);
					
					t->MagicID = TDELETED_MAGICID;
					Remove((struct Node *)t);
					Free(t);
				}
				else ReleaseSemaphore(&t->sem);
			}
			else
			{
				DBG(" *** Invalid thread attached to list !? \a\n");
				Remove((struct Node *)t);
			}
			
			node = succ;
		}
		
		while((tmsg = (ThreadMessage *) GetMsg(m->sigport)))
		{
			if(tmsg->replyed == FALSE)
			{
				DBG_THREAD(tmsg->supervisor);
				
				t = tmsg->supervisor;
				
				if(ATHREAD_VALID(t))
				{
					tHandler th;
					
					if((th = t->handler))
					{
						th(t,tmsg->flags,tmsg->udata);
					}
				}
				tmsg->replyed = TRUE;
				ReplyMsg((struct Message *)tmsg);
			}
			else Free(tmsg);
		}
	}
	
	LEAVE();
}

//------------------------------------------------------------------------------

BOOL PutThreadMsg(Thread *t, struct MsgPort *from, 
	struct MsgPort *to, UWORD flags, APTR udata)
{
	BOOL rc = FALSE;
	
	ENTER();
	DBG_THREAD(t);
	
	if(ATHREAD_VALID(t))
	{
		ThreadMessage * tmsg;
		
		ObtainSemaphore(&t->sem);
		
		if((tmsg = Malloc(sizeof(*tmsg))))
		{
			bZero( tmsg, sizeof(*tmsg));
			
			tmsg->msg.mn_Length = sizeof(*tmsg);
			tmsg->msg.mn_ReplyPort = from;
			tmsg->supervisor = t;
			tmsg->flags = flags;
			tmsg->udata = udata;
			PutMsg( to, (struct Message *)tmsg);
			rc = TRUE;
		}
		
		ReleaseSemaphore(&t->sem);
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------



