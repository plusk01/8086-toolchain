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

/*
 *  This module provide the routine to list the contents of the
 *  symbol table.
 */

/*****************************************************************************/

#include "chdr.h"
#ifdef LIST
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*********************************************** Static Function Definitions */

static void put_sc P_ ((STORAGE));
static void put_def P_ ((STATUS));
static void put_ty P_ ((const TYP *, LEVEL));
static void list_var P_ ((const SYM *, LEVEL));
static void list_table P_ ((const TABLE *, LEVEL));
static void list_block P_ ((const BLOCK *, LEVEL));

/*****************************************************************************/

static void put_def P1 (STATUS, status)
{
    lprintf (status & (SYM_DEFINED) ? "D" : " ");
    lprintf (status & (SYM_SET) ? "S" : " ");
    lprintf (status & (SYM_USED) ? "U" : " ");
    lprintf (status & (SYM_OUTSCOPE) ? "O" : " ");
    lprintf (status & (SYM_OUTPUT) ? "P" : " ");
    lprintf (" ");
}

static void put_sc P1 (STORAGE, scl)
{
    switch (scl) {
    case sc_static:
	lprintf ("Static      ");
	break;
    case sc_auto:
	lprintf ("Auto        ");
	break;
    case sc_global:
	lprintf ("Global      ");
	break;
    case sc_external:
	lprintf ("External    ");
	break;
    case sc_tag:
	lprintf ("Tag         ");
	break;
    case sc_typedef:
	lprintf ("Typedef     ");
	break;
    case sc_const:
	lprintf ("Constant    ");
	break;
    case sc_member:
	lprintf ("Member      ");
	break;
    case sc_label:
	lprintf ("Label       ");
	break;
    case sc_parms:
	lprintf ("Parameter   ");
	break;
    case sc_register:
	lprintf ("Register   ");
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
}

static void put_fs P1 (FSPECIFIER, fs)
{
    switch (fs) {
    case fs_none:
	break;
    case fs_inline:
	lprintf ("inline ");
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
}

static void put_ty P2 (const TYP *, tp, LEVEL, indent)
{
    if (tp == NIL_TYP) {
	return;
    }
    if (!is_unknown_size (tp)) {
	lprintf ("%lx:", tp->size);
    }
    if (is_const_qualified (tp)) {
	lprintf ("const ");
    }
    if (is_volatile_qualified (tp)) {
	lprintf ("volatile ");
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
	lprintf ("%s ", nameoftype (tp));
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	    lprintf ("array of ");
	} else {
	    lprintf ("pointer to ");
	}
	put_ty (referenced_type (tp), indent);
	break;
    case bt_func:
	lprintf ("function returning ");
	put_ty (returned_type (tp), indent);
	break;
    case bt_bbitfield:
	lprintf ("_Bool ");
	goto bitfield;   /*lint !e801*/  /* use of goto is deprecated */
    case bt_ubitfield:
	lprintf ("unsigned ");
	/*lint -fallthrough */
    case bt_bitfield:
      bitfield:
	lprintf ("bitfield offset=%d width=%d",
		 (int) bitfield_offset (tp), (int) bitfield_width (tp));
	break;
    default:
	CANNOT_REACH_HERE ();
    }

    if (is_integral_type (tp)) {
	RANGE  *rp = rangeof (tp);

	if (is_signed_type (tp)) {
	    lprintf ("[%ld..%ld]", rp->low, rp->high);
	} else {
	    lprintf ("[0x%lx..0x%lx]", (unsigned long) rp->low,
		     (unsigned long) rp->high);
	}
    }
}

static void list_var P2 (const SYM *, sp, LEVEL, indent)
{
    LEVEL   j;

    for (j = indent; j; --j)
	lprintf ("    ");
    lprintf ("%-12s =0x%08lx ", nameof (sp), sp->value.u);
    put_sc (storageof (sp));
    put_fs (fspecifierof (sp));
    put_def (sp->status);
    put_ty (typeof (sp), indent);
    lprintf ("%s", newline);
    if (typeof (sp) != NIL_TYP) {
	switch (typeof (sp)->type) {
	case bt_struct:
	case bt_union:
	    if (is_tag (sp)) {
		list_block (members (typeof (sp)), indent + (LEVEL) 1);
	    }
	    break;
	case bt_func:
	    list_block (parameters (typeof (sp)), indent + (LEVEL) 1);
	    break;
	default:
	    break;
	}
    }
}

static void list_table P2 (const TABLE *, t, LEVEL, indent)
{
    if (t != NIL_TABLE) {
	SYM    *sp;

	for (sp = t->head; sp != NIL_SYM; sp = nextsym (sp))
	    list_var (sp, indent);
    }
}

static void list_block P2 (const BLOCK *, block, LEVEL, indent)
{
    if (block != NIL_BLOCK) {
	list_table (&(block->symbols), indent);
	list_table (&(block->tags), indent);
    }
}

void summary P2 (const BLOCK *, block, LEVEL, indent)
{
    if (listing_option) {
	list_block (block, indent);
    }
}
#endif /* LIST */
