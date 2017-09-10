/*
 * C compiler
 * ==========
 *
 * Copyright 1989, 1990, 1991 Christoph van Wuellen.
 * Credits to Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 */

/*****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "outproto.h"

#ifdef CPU_DEFINED

/*****************************************************************************/
static void geninit P_ ((const EXPR *, unsigned char *));


/*****************************************************************************/ 

static void geninittype P2 (const EXPR *, ep, unsigned char *, data)
{
    TYP    *tp = ep->etp;
    UVAL    uval;

    
#ifdef FLOAT_SUPPORT
    RVAL    rvl;

    
#endif /* FLOAT_SUPPORT */
	UNUSEDARG (data);
    switch (tp->type) {
    case bt_bool:
	put_char (ep);
	break;
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	put_char (ep);
	break;
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
	put_short (ep);
	break;
    case bt_pointer16:
    case bt_pointer32:
	break;
    case bt_bitfield:
    case bt_ubitfield:
    case bt_bbitfield:
	uval = ep->v.u & bitmask (bitfield_width (tp));
	break;
    case bt_int32:
    case bt_uint32:
    case bt_ulong:
    case bt_long:
	put_long (ep);
	break;
    case bt_struct:
	break;
    case bt_union:
	break;
	
#ifdef FLOAT_SUPPORT
    case bt_float:
	floatexpr (tp, &rvl);
	put_float (&rvl);
	break;
    case bt_double:
	floatexpr (tp, &rvl);
	put_double (&rval);
	break;
    case bt_longdouble:
	floatexpr (tp, &rvl);
	put_longdouble (&rvl);
	break;
	
#endif /* FLOAT_SUPPORT */
    case bt_func:
	break;
    default:
	break;
    }
}

static void geninit P2 (const EXPR *, ep, unsigned char *, data)
{
    UNUSEDARG (data);
    if (!code_option)
	return;

    switch (ep->nodetype) {
    case en_list:
	for (; ep != NIL_EXPR; ep = ep->v.p[1]) {
	    geninit (ep->v.p[0], data);
	}
	break;
    case en_icon:
    case en_fcon:
    case en_str:
	geninittype (ep, data);
	break;
    case en_litval:
	geninit (ep->v.p[0], data);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

void g_init P1 (const EXPR *, ep)
{
    unsigned char *data;
    SIZE    size;

    switch (ep->nodetype) {
    case en_literal:
	size = (SIZE) ep->v.p[1]->v.i;
	data = xalloc ((size_t) size);
	VOIDCAST memset (data, 0, (size_t) size);

	geninit (ep->v.p[0], data);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

#endif /* CPU_DEFINED */
