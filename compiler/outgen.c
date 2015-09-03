/*      o u t g e n . c

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

/******************************************************************************
 *
 * This module contains code generation routines that are common to
 * both 68k and 386 processors, and also to all target assembler
 * types.
 *
 * Previously these routines were repeasted in all of the following
 * modules:
 *              out386_bas.c
 *              out386_gas.c
 *              out68k_ack.c
 *              out68k_cpm.c
 *              out68k_gas.c
 *****************************************************************************/

#include "chdr.h"
#ifdef CPU_DEFINED
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*
 * This module can be set up to prepend underscores to the
 * names shipped out by this module.
 * Names starting with a dot are not translated, theses are
 * compiler support functions which should not fall in the
 * user's namespace
 *
 * Capable of translating symbol names before output, since external
 * symbols are truncated to eight characters adopt the algorithm used in
 * ,,scanaout''
 */

/*****************************************************************************/

const CHAR *outlate P1 (const CHAR *, string)
{
    static CHAR symname[MAX_ID_LEN + 1];
    const CHAR *sp;
    CHAR   *dp;

#ifdef TRANSLATE
    int     k;

#endif /* TRANSLATE */

    dp = (CHAR *) &symname[0];
    if (*string == (CHAR) '.') {
#ifdef TRANSLATE
	if (!trans_option) {
	    return string;
	}
	/* delete the leading '.' which mucks things up here!! */
	sp = &string[1];
#else
	return string;
#endif /* TRANSLATE */
    } else {
	sp = external_prefix;
	while (*sp) {
	    *dp++ = (CHAR) *sp++;
	};
	sp = string;
    }
    while ((*dp++ = (CHAR) *sp++) != (CHAR) 0)
	continue;

#ifdef TRANSLATE
    if (trans_option && ((k = (int) strlen ((char *) symname)) > 8)) {
	/*
	 * translate long symbol name to 8 chars 140603: big prime number
	 * smaller than 52**3 This is for defective assemblers/linkers that
	 * can only handle external names which are 8 chars wide, this may
	 * help if you have problems with different identifiers not
	 * being different in the first 7 (seven, due to the leading
	 * underscore) characters
	 *
	 * THIS IS UGLY, BUT IT IT THE LAST RESORT, AND IT HELPED ME IN SOME
	 * CASES. TRANS_OPTION IS *NOT* THE DEFAULT
	 */
	unsigned long check;
	int     j;

	check = (unsigned long) 0;
	for (j = 0; j < k; j++)
	    check =
		(check + check +
		 (unsigned long) symname[j]) % (unsigned long) 140603L;
	j = (int) (check % (unsigned long) 52);
	check = check / (unsigned long) 52;
	symname[0] = (CHAR) ((j < 26) ? 'a' + j : 'A' + j - 26);
	j = (int) (check % (unsigned long) 52);
	check = check / (unsigned long) 52;
	symname[1] = (CHAR) ((j < 26) ? 'a' + j : 'A' + j - 26);
	j = (int) (check % (unsigned long) 52);
	symname[2] = (CHAR) ((j < 26) ? 'a' + j : 'A' + j - 26);
	symname[8] = (CHAR) '\0';
    }
#endif /* TRANSLATE */
    return symname;
}
#endif /* CPU_DEFINED */
