/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2006, Diego Casorran <dcasorran@gmail.com>
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
 * $Id: debug.h,v 0.1 2006/03/14 11:28:57 diegocr Exp $
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
# include <clib/debug_protos.h>

static __inline void __dEbuGpRInTF( CONST_STRPTR func_name, CONST LONG line )
{
	KPutStr( func_name );
	KPrintF(",%ld: ", line );
}
# define DBG( fmt... ) \
({ \
	__dEbuGpRInTF(__PRETTY_FUNCTION__, __LINE__); \
	KPrintF( fmt ); \
})
# define DBG_POINTER( ptr )	DBG("%s = 0x%08lx\n", #ptr, (long) ptr )
# define DBG_VALUE( val )	DBG("%s = 0x%08lx,%ld\n", #val,val,val )
# define DBG_STRING( str )	DBG("%s = 0x%08lx,\"%s\"\n", #str,str,str)
# define DBG_ASSERT( expr )	if(!( expr )) DBG(" **** FAILED ASSERTION '%s' ****\n", #expr )
# define DBG_EXPR( expr, bool )						\
({									\
	BOOL res = (expr) ? TRUE : FALSE;				\
									\
	if(res != bool)							\
		DBG("Failed %s expression for '%s'\n", #bool, #expr );	\
									\
	res;								\
})
#else
# define DBG(fmt...)		((void)0)
# define DBG_POINTER( ptr )	((void)0)
# define DBG_VALUE( ptr )	((void)0)
# define DBG_STRING( str )	((void)0)
# define DBG_ASSERT( expr )	((void)0)
# define DBG_EXPR( expr, bool )	expr
#endif

#define DBG_TRUE( expr )	DBG_EXPR( expr, TRUE )
#define DBG_FALSE( expr )	DBG_EXPR( expr, FALSE)

#define ENTER()		DBG("--- ENTERING %s:%ld\n", __func__, __LINE__)
#define LEAVE()		DBG("--- LEAVING %s:%ld\n", __func__, __LINE__)
#define RETURN(Rc)	\
	DBG("--- LEAVING %s:%ld RC=%ld,0x%08lx\n", __func__, __LINE__, (LONG)(Rc),(LONG)(Rc))

#endif /* DEBUG_H */
