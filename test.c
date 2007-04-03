
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include "palloc.h"
#include "athread.h"
#include "usema.h"

int __nocommandline = 1;

BOOL qtfunc( Thread *t)
{
	ENTER();
	DBG_POINTER(t);
	
	if( t != NULL )
	{
		DBG_THREAD(t);
		
		if(t->manager == NULL)
		{
			DBG("no manager attached to this thread !?\n");
		}
		else if(!VALID_TMANAGER(t->manager))
		{
			DBG(" ---------- handshaking error\n");
		}
		else
		{
			DBG_POINTER(t->task);
			DBG_POINTER(t->port);
			DBG_POINTER(t->parent);
			DBG_POINTER(t->parent_port);
			
			DBG_POINTER(t->func);
			DBG_POINTER(t->handler);
			
			DBG_POINTER(t->args);
			
			while(*t->args)
			{
				DBG_POINTER(*t->args);
				t->args++;
			}
		}
	}
	
	LEAVE();
	return TRUE;
}

VOID at_handler(Thread *t,int flags,APTR data)
{
	ENTER();
	DBG_POINTER(t);
	DBG_VALUE(flags);
	DBG_POINTER(data);
	
	
	
	LEAVE();
}

BOOL at_func(Thread *t)
{
	ULONG sigs;
	
	ENTER();
	DBG_POINTER(t);
	DBG_POINTER(t->args);
	DBG_POINTER(*t->args); // <- this is the manager
	DBG_ASSERT(*t->args == (ULONG)t->manager);
	
	PutThreadMsg( t, t->port, t->manager->sigport, 12345, t );
again:
	DBG(" +++++ Waiting...\n");
	
	sigs = Wait( SIGBREAKF_CTRL_C|(1L<<t->port->mp_SigBit));
	
	if(sigs & (1L<<t->port->mp_SigBit))
	{
		/* this is the PutThreadMsg() reply! */
		DBG(" ---- port signal, obtaining uSema...\n");
		uSemaLock();
		DBG("++++++++++++++ GOT uSema, inlocking...\n");
		uSemaUnLock();
		goto again;
	}
	
	LEAVE();
	return TRUE;
}



struct asdf { char buf[16]; };

#define chk(m) do {\
	if(!m) { \
		DBG("no mem for %s at %s:%ld\n",#m,__func__,__LINE__); \
		return; \
	} \
} while(0)

void test_memory(void)
{
	struct asdf *d, *b;
	
	// test the avoid freeing unknown memory..
	DBG("there must be two memory errors:\n");
	Free(qtfunc);
	Realloc( test_memory, 1025);
	
	DBG("Allocating 'd'\n");
	d = Malloc(sizeof(*d)); chk(d);
	DBG_POINTER(d);
	CopyMem( "doom\0", d->buf, 5 );
	
	DBG("Reallocating 'd' to 'b'\n");
	b = Realloc( d, sizeof(*d)+33);
	DBG_POINTER(b);
	DBG_STRING( b->buf );
	
	DBG("Trying to access 'd' which was freed on Realloc()\n");
	DBG_STRING( d->buf );
	Delay(8);
	
	DBG("Freeing memory, there must be A error:\n");
	Free(d); // <- this should fails
	Free(b);
	
	DBG("Memory test completed\n");
}

int main( /*int argv, char ** argc*/ )
{
	int rc = 0;
	
	ENTER();
	
	if(InitMemoryPool())
	{
		ThreadManager * tm;
		
		//test_memory();
		
		if(uSemaInit())
		{
			if((tm = InitThreadManager(0)))
			{
				int x = 1;
				Thread * t;
				
				//qThread( tm, qtfunc );
				t = ThreadLaunch(tm,".......hello world",-2,at_handler,at_func,(ULONG)tm,TAG_DONE);
				
				uSemaLock();
				
				do {
					DBG("Loop...\n");
					
					ULONG sigs = Wait( tm->sigmask | SIGBREAKF_CTRL_C);
					
					if(sigs & SIGBREAKF_CTRL_C)
					{
						DBG("ctrl+c\n");
						
						if(!x--) break;
						
						uSemaUnLock();
						PutThreadMsg( t, tm->sigport, t->port, 12345, t );
					}
					
					if( sigs & tm->sigmask )
					{
						ThreadManagerDispatcher( tm );
					}
				} while(1);
				
				CleanupThreadManager(tm);
			}
			
			uSemaClear();
		}
		
		ClearMemoryPool();
	}
	
	RETURN(rc);
	return(rc);
}
