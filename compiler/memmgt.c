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
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/********************************************************* Macro Definitions */

/*
 * Memory is allocated in Blocks of 1024 longs, which form a linked list
 */
#define BLKLEN ((size_t)1024 * sizeof(long))
#define NIL_BLK ( (struct blk *) 0)

/********************************************************** Type Definitions */

struct blk
{
    struct blk *next;		/* next allocated block */
};

/********************************************************** Static Variables */

static size_t glbsize = (size_t) 0;	/* size left in current global block */
static size_t locsize = (size_t) 0;	/* size left in current local block */
static char *glbdata = (char *) 0;	/* global index */
static char *locdata = (char *) 0;	/* local index */

/* statistics... */
static size_t max_mem = (size_t) 0;
static size_t glo_mem = (size_t) 0;
static size_t loc_mem = (size_t) 0;

static struct blk *locblk = NIL_BLK;	/* pointer to local block list */
static struct blk *glbblk = NIL_BLK;	/* pointer to global block list */

/*****************************************************************************/

VOIDSTAR xalloc P1 (size_t, siz)
{
    struct blk *bp;
    void   *rv;
    size_t  len;

    /*
     * DO NOT use AL_DEFAULT here: host and target machine may differ.
     */
    if (siz & (size_t) (AL_HOST - 1L)) {
	siz += (size_t) AL_HOST - (siz & (size_t) (AL_HOST - 1L));
    }
    if (global_flag) {
	if (siz > glbsize) {
	    len = (siz > BLKLEN) ? siz : BLKLEN;
	    bp = (struct blk *) malloc (len + sizeof (struct blk));

	    if (bp == NULL) {
		message (MSG_NOMEMORY, (int) (glo_mem * sizeof (long)),
			 (int) (loc_mem * sizeof (long)));

		exit (EXIT_FAILURE);
	    }
	    glo_mem++;
	    bp->next = glbblk;
	    glbblk = bp;
	    glbsize = len;
	    glbdata = (char *) bp + sizeof (struct blk);
	}
	rv = glbdata;
	glbdata += siz;
	glbsize -= siz;
	return rv;
    } else {			/* not global */
	if (siz > locsize) {
	    len = (siz > BLKLEN) ? siz : BLKLEN;
	    bp = (struct blk *) malloc (len + sizeof (struct blk));

	    if (bp == NULL) {
		message (MSG_LOCALMEM, (int) (loc_mem * sizeof (long)),
			 (int) (glo_mem * sizeof (long)));

		exit (EXIT_FAILURE);
	    }
	    loc_mem++;
	    bp->next = locblk;
	    locblk = bp;
	    locsize = len;
	    locdata = (char *) bp + sizeof (struct blk);
	}
	rv = locdata;
	locdata += siz;
	locsize -= siz;
	return rv;
    }
}

VOIDSTAR galloc P1 (size_t, siz)
{
    void   *ptr;

    global_flag++;
    ptr = xalloc (siz);
    global_flag--;
    return ptr;
}

void rel_local P0 (void)
{
    struct blk *bp1, *bp2;

    bp1 = locblk;
    while (bp1 != NIL_BLK) {
	bp2 = bp1->next;
	free (bp1);
	bp1 = bp2;
    }
    if (loc_mem + glo_mem > max_mem) {
	max_mem = loc_mem + glo_mem;
    }
#ifdef LIST
#ifdef VERBOSE
    if (listing_option && verbose_option) {
	message (MSG_RELEASELOC, (int) (loc_mem * sizeof (long)));
    }
#endif /* VERBOSE */
#endif /* LIST */
    locblk = NIL_BLK;
    loc_mem = (size_t) 0;
    locsize = (size_t) 0;
}

void rel_global P0 (void)
{
    struct blk *bp1, *bp2;

    bp1 = glbblk;
    while (bp1 != NIL_BLK) {
	bp2 = bp1->next;
	free (bp1);
	bp1 = bp2;
    }
    if (glo_mem > max_mem) {
	max_mem = glo_mem;
    }
#ifdef VERBOSE
    if (verbose_option) {
#ifdef LIST
	if (listing_option) {
	    message (MSG_RELEASEGLB, (int) (glo_mem * sizeof (long)));
	}
#endif /* LIST */
	message (MSG_MAXMEMORY, (int) (max_mem * sizeof (long)));
    }
#endif
    glbblk = NIL_BLK;
    max_mem = (size_t) 0;
    glo_mem = (size_t) 0;
    glbsize = (size_t) 0;
}
