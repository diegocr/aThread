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
 * $Id: debug.c,v 0.1 2006/03/14 11:28:57 diegocr Exp $
 * 
 * ***** END LICENSE BLOCK ***** */

#ifdef DEBUG
# include <proto/exec.h>
# include <stdarg.h>
# include <SDI_compiler.h> /* REG() macro */

#ifndef RawPutChar
# define RawPutChar(CH)	LP1NR( 0x204, RawPutChar, ULONG, CH, d0,,SysBase)
#endif

VOID KPutC(UBYTE ch)
{
	RawPutChar(ch);
}

VOID KPutStr(CONST_STRPTR string)
{
	UBYTE ch;
	
	while((ch = *string++))
		KPutC( ch );
}

STATIC VOID ASM RawPutC(REG(d0,UBYTE ch))
{
	KPutC(ch); 
}

VOID KPrintF(CONST_STRPTR fmt, ...)
{
	va_list args;
	
	va_start(args,fmt);
	RawDoFmt((STRPTR)fmt, args,(VOID (*)())RawPutC, NULL );
	va_end(args);
}

#endif /* DEBUG */
