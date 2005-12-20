/*
 * Copyright (c) 2003 Matteo Frigo
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Id: print.c,v 1.28 2005-12-20 02:27:25 athena Exp $ */

#include "ifftw.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#define BSZ 64

static void myputs(printer *p, const char *s)
{
     char c;
     while ((c = *s++))
          p->putchr(p, c);
}

static void newline(printer *p)
{
     int i;

     p->putchr(p, '\n');
     for (i = 0; i < p->indent; ++i)
	  p->putchr(p, ' ');
}

static void putint(printer *p, INT i, INT base)
{
     char buf[BSZ];
     char *f = buf;

     if (i < 0) {
	  p->putchr(p, '-');
	  i = -i;
     }
     
     do {
	  *f++ = "0123456789abcdef"[i % base];
	  i /= base;
     } while (i);
     
     do {
	  p->putchr(p, *--f);
     } while (f != buf);
}

static void vprint(printer *p, const char *format, va_list ap)
{
     char buf[BSZ];
     const char *s = format;
     char c;

     while ((c = *s++)) {
          switch (c) {
	      case '%':
		   switch ((c = *s++)) {
		       case 'M': {
			    /* md5 value */
			    md5uint x = va_arg(ap, md5uint);
			    x = 0xffffffffUL & x;
			    sprintf(buf, "%8.8lx", (unsigned long)x);
			    goto putbuf;
		       }
		       case 'c': {
			    int x = va_arg(ap, int);
			    p->putchr(p, x);
			    break;
		       }
		       case 's': {
			    char *x = va_arg(ap, char *);
			    if (x)
				 myputs(p, x);
			    else
				 goto putnull;
			    break;
		       }
		       case 'd': {
			    int x = va_arg(ap, int);
			    sprintf(buf, "%d", x);
			    goto putbuf;
		       }
		       case 'D': {
			    INT x = va_arg(ap, INT);
			    putint(p, x, 10);
			    break;
		       }
		       case 'f': case 'e': case 'g': {
			    char fmt[3] = "%x";
			    double x = va_arg(ap, double);
			    fmt[1] = c;
			    sprintf(buf, fmt, x);
			    goto putbuf;
		       }
		       case 'v': {
			    /* print optional vector length */
			    INT x = va_arg(ap, INT);
			    if (x > 1) {
				 myputs(p, "-x");
				 putint(p, x, 10);
			    }
			    break;
		       }
		       case 'o': {
			    /* integer option.  Usage: %oNAME= */
			    INT x = va_arg(ap, INT);
			    if (x)
				 p->putchr(p, '/');
			    while ((c = *s++) != '=')
				 if (x)
				      p->putchr(p, c);
			    if (x) {
				 p->putchr(p, '=');
				 putint(p, x, 10);
			    }
			    break;
		       }
		       case 'u': {
			    unsigned x = va_arg(ap, unsigned);
			    sprintf(buf, "%u", x);
			    goto putbuf;
		       }
		       case 'x': {
			    unsigned x = va_arg(ap, unsigned);
			    sprintf(buf, "%x", x);
			    goto putbuf;
		       }
		       case '(': {
			    /* newline, augment indent level */
			    p->indent += p->indent_incr;
			    newline(p);
			    break;
		       }
		       case ')': {
			    /* decrement indent level */
			    p->indent -= p->indent_incr;
			    break;
		       }
		       case 'p': {  /* note difference from C's %p */
			    /* print plan */
			    plan *x = va_arg(ap, plan *);
			    if (x) 
				 x->adt->print(x, p);
			    else 
				 goto putnull;
			    break;
		       }
		       case 'P': {
			    /* print problem */
			    problem *x = va_arg(ap, problem *);
			    if (x)
				 x->adt->print(x, p);
			    else
				 goto putnull;
			    break;
		       }
		       case 'T': {
			    /* print tensor */
			    tensor *x = va_arg(ap, tensor *);
			    if (x)
				 X(tensor_print)(x, p);
			    else
				 goto putnull;
			    break;
		       }
		       default:
			    A(0 /* unknown format */);
			    break;

		   putbuf:
			    myputs(p, buf);
			    break;
		   putnull:
			    myputs(p, "(null)");
			    break;
		   }
		   break;
	      default:
		   p->putchr(p, c);
		   break;
          }
     }
}

static void print(printer *p, const char *format, ...)
{
     va_list ap;
     va_start(ap, format);
     vprint(p, format, ap);
     va_end(ap);
}

printer *X(mkprinter)(size_t size, 
		      void (*putchr)(printer *p, char c),
		      void (*cleanup)(printer *p))
{
     printer *s = (printer *)MALLOC(size, OTHER);
     s->print = print;
     s->vprint = vprint;
     s->putchr = putchr;
     s->cleanup = cleanup;
     s->indent = 0;
     s->indent_incr = 2;
     return s;
}

void X(printer_destroy)(printer *p)
{
     if (p->cleanup)
	  p->cleanup(p);
     X(ifree)(p);
}
