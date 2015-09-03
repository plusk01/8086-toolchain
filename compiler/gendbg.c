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
 *		  originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *		  started i386 port (December)
 * 1991   i386 port finished (January)
 *		  further corrections in the front end and in the 68000
 *		  code generator.
 *		  The next port will be a SPARC port
 */

/*****************************************************************************/

#include "config.h"
#include "chdr.h"

#ifdef DEBUGINFO
void gen_type P1 (const TYP *, tp)
{
    if (tp == NIL_TYP) {
	return;
    }
    if (is_const_qualified (tp)) {
    }
    if (is_volatile_qualified (tp)) {
    }
    switch (tp->type) {
    case bt_void:
    case bt_bool:
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
    case bt_double:
    case bt_longdouble:
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
    case bt_floatimaginary:
    case bt_doubleimaginary:
    case bt_longdoubleimaginary:
    case bt_union:
    case bt_struct:
    case bt_ellipsis:
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	} else {
	}
	gen_type (referenced_type (tp));
	break;
    case bt_func:
	gen_type (returned_type (tp));
	break;
    case bt_bbitfield:
	goto bitfield;
    case bt_ubitfield:
	/*FALLTHRU */
    case bt_bitfield:
      bitfield:
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}
#endif /* DEBUGINFO */
