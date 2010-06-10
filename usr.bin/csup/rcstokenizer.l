/*-
 * Copyright (c) 2007-2009, Ulf Lilleengen <lulf@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.bin/csup/rcstokenizer.l,v 1.1 2010/03/02 07:26:07 lulf Exp $
 */

/*
 * This tokenizer must be generated by a lexxer with support for reentrancy.
 */
%{
#include <string.h>

#include "misc.h"
#include "rcsparse.h"

%}
%option reentrant noyywrap
%option header-file="rcstokenizer.h"

everything	(.|\n)*
num		[0-9\.]+
whitespace	[\t\n ]
digit		[0-9]
idchar		[^$,.:;\t\n ]
string		@([^@]|\n|"@@")*@
keyword		head|access|symbols|locks|comment|expand|strict|date|author|state|branches|next|desc|log|text
keyword2	branch
newline		\n
%%

{keyword2}	{
	return (KEYWORD_TWO);
}
{keyword}	{
	return (KEYWORD);
}
{string}	{
	return (STRING);
}
{num}		{
	return (NUM);
}
{num}?{idchar}({idchar}|{num})* {
/* This will use ID as both ID and SYM. Do extra checking elsewhere.*/
	return (ID);
}
;	{ return (SEMIC); }
:	{ return (COLON); }
\n	;
[ \t]+	;
%%