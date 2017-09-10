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
 * History:
 *
 * 1989   starting an 68000 C compiler, starting with material
 *        originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *        started i386 port (December)
 * 1991   i386 port finished (January)
 *        further corrections in the front end and in the 68000
 *        code generator.
 *        The next port will be a SPARC port
 */

/*****************************************************************************/

#include "chdr.h"
#ifdef EXTERNAL
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*********************************************** Static Function Definitions */

static void put_parms P_ ((const BLOCK *));
static void put_typ P_ ((const TYP *));

/*****************************************************************************/

static void put_parms P1 (const BLOCK *, block)
{
    lprintf ("(");
    if (block != NIL_BLOCK) {
	SYM    *sp = symbolsof (block);

	if (sp != NIL_SYM) {
	    for (;;) {
		put_typ (typeof (sp));
		sp = nextsym (sp);
		if (sp == NIL_SYM) {
		    break;
		}
		lprintf (",");
	    }
	}
    }
    lprintf (")");
}

static void put_typ P1 (const TYP *, tp)
{
    if (tp == NIL_TYP) {
	return;
    }
    if (is_const_qualified (tp)) {
	lprintf ("const ");
    }
    if (is_volatile_qualified (tp)) {
	lprintf ("volatile ");
    }
    switch (tp->type) {
    case bt_void:
    case bt_schar:
    case bt_uchar:
    case bt_char:
    case bt_charu:
    case bt_ushort:
    case bt_short:
    case bt_uint16:
    case bt_uint32:
    case bt_int16:
    case bt_int32:
    case bt_ulong:
    case bt_long:
    case bt_float:
    case bt_longdouble:
    case bt_double:
    case bt_ellipsis:
	lprintf ("%s", nameoftype (tp));
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	    put_typ (referenced_type (tp));
	    lprintf ("[]");
	} else {
	    put_typ (referenced_type (tp));
	    lprintf ("*");
	}
	break;
    case bt_union:
	lprintf ("union");
	goto ucont;	    /*lint !e801*/  /* use of goto is deprecated */
    case bt_struct:
	lprintf ("struct");
      ucont:
	if (nameoftype (tp) != NIL_CHAR) {
	    lprintf (" %s", nameoftype (tp));
	}
	break;
    case bt_func:
	put_typ (returned_type (tp));
	put_parms (parameters (tp));
	break;
    case bt_bitfield:
    case bt_ubitfield:
    case bt_bbitfield:
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}


void funclist P1 (const SYM *, sp)
{
    if (extern_option) {
	put_typ (returned_type (typeof (sp)));
	lprintf (" %s", nameof (sp));
	put_parms (parameters (typeof (sp)));
	lprintf (";%s", newline);
    }
}
#endif /* EXTERNAL */
