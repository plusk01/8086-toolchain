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

#ifdef DWARF2
static void initialise P0 (void)
{
#if 0
	put_seg (".debug_info", 0);
	put_seg (".debug_abbrev", 0);
	put_seg (".debug_line", 0);
	put_seg (".debug_frame", 0);
	put_seg (".debug_aranges", 0);
	put_seg (".debug_pubnames", 0);
	put_srg (".debug_string", 0);
#endif
}

void put_void P0 (void)
{
}
#endif /* DWARF2 */
