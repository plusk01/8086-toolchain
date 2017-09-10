/*
   **   This module builds the compiler as one large C file.   Maybe the
   **   optimizer can do a better job in this case!
   **
   **   This module will only work if only one code generator and one
   **   assembler format is selected.
 */

#include "config.h"
#include "check.h"

#ifdef MULTIPLE_ASSEMBLERS
error, define only one assembler format
#endif /* MULTIPLE_ASSEMBLERS */

#ifdef MULTIPLE_PROCESSORS
error.define only one processor type
#endif /* MULTIPLE_PROCESSORS */

#include "analyze.c"
#include "cglbdef.c"
#include "cmain.c"
#include "decl.c"
#include "expr.c"
#include "extern.c"
#include "geninit.c"
#include "genicode.c"
#include "genstmt.c"
#include "genutil.c"
#include "getsym.c"
#include "init.c"
#include "inline.c"
#include "intexpr.c"
#include "list.c"
#include "memmgt.c"
#include "msgout.c"
#include "optimize.c"
#include "outgen.c"
#include "stmt.c"
#include "symbol.c"
#include "system.c"
#include "types.c"

#include "genffp.c"
#include "genieee.c"

#include "flow68k.c"
#include "gen68k.c"
#include "peep68k.c"
#include "out68k_a.c"
#include "out68k_c.c"
#include "out68k_g.c"
#include "out68k_q.c"
#include "reg68k.c"

#include "peepX86.c"
#include "regX86.c"
#include "outX86_a.c"
#include "outX86_b.c"
#include "outX86_g.c"
#include "outX86_s.c"
#include "gen386.c"
#include "gen86.c"

#include "genarm.c"
#include "peeparm.c"
#include "outarm_obj.c"
#include "regarm.c"

#include "flowc30.c"
#include "genc30.c"
#include "peepc30.c"
#include "outc30_r.c"
#include "reg68k.c"

#include "genppc.c"
#include "peepppc.c"
#include "regppc.c"
