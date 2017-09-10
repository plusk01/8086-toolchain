/* ldrdf.c      RDOFF Object File linker/loader main program
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 */

/*
 * TODO:
 *   enhance search of required export symbols in libraries (now depends
 *   on modules order in library)
 * - keep a cache of symbol names in each library module so
 *   we don't have to constantly recheck the file
 * - general performance improvements
 *
 * BUGS & LIMITATIONS: this program doesn't support multiple code, data
 * or bss segments, therefore for 16 bit programs whose code, data or BSS
 * segment exceeds 64K in size, it will not work. This program probably
 * won't work if compiled by a 16 bit compiler. Try DJGPP if you're running
 * under DOS. '#define STINGY_MEMORY' may help a little.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multboot.h"
#include "rdoff.h"
#include "symtab.h"
#include "collectn.h"
#include "rdlib.h"
#include "segtab.h"

#define LDRDF_VERSION "1.04"

#define RDF_MAXSEGS 64
/* #define STINGY_MEMORY */

/* =======================================================================
 * Types & macros that are private to this program
 */

struct segment_infonode {
    int		dest_seg;	/* output segment to be placed into, -1 to 
				   skip linking this segment */
    long	reloc;		/* segment's relocation factor */
};


struct modulenode {
    rdffile	f;		/* the RDOFF file structure */
    struct segment_infonode seginfo[RDF_MAXSEGS]; /* what are we doing
						     with each segment? */
    void	* header;
    char	* name;
    struct modulenode * next;
    long	bss_reloc;
};

#include "ldsegs.h"

#define newstr(str) strcpy(malloc(strlen(str) + 1),str)
#define newstrcat(s1,s2) strcat(strcpy(malloc(strlen(s1)+strlen(s2)+1),s1),s2)

/* ==========================================================================
 * Function prototypes of private utility functions
 */

void processmodule(const char * filename, struct modulenode * mod);
int allocnewseg(int16 type,int16 reserved);
int findsegment(int16 type,int16 reserved);
void symtab_add(const char * symbol, int segment, long offset);
int symtab_get(const char * symbol, int * segment, long * offset);

/* =========================================================================
 * Global data structures.
 */

/* a linked list of modules that will be included in the output */
struct modulenode 	* modules = NULL;
struct modulenode 	* lastmodule = NULL;

/* a linked list of libraries to be searched for unresolved imported symbols */
struct librarynode 	* libraries = NULL;
struct librarynode	* lastlib = NULL;

/* the symbol table */
void 			* symtab = NULL;

/* objects search path */
char			* objpath = NULL;

/* libraries search path */
char			* libpath = NULL;

/* error file */
static FILE		* error_file;

/* the header of the output file, built up stage by stage */
rdf_headerbuf 		* newheader = NULL;

/* The current state of segment allocation, including information about
 * which output segment numbers have been allocated, and their types and
 * amount of data which has already been allocated inside them. 
 */
struct SegmentHeaderRec	outputseg[RDF_MAXSEGS];
int			nsegs = 0;
long			bss_length;

/* global options which affect how the program behaves */
struct ldrdfoptions {
    int		verbose;
    int		align;
    int		warnUnresolved;
    int		errorUnresolved;
    int		strip;
    int         respfile;
    int         stderr_redir;
    int         objpath;
    int         libpath;
    int		addMBheader;
} options;

int errorcount = 0;	/* determines main program exit status */

/*
 * Multiboot header support.
 */
 
/* loading address for multiboot header */
unsigned		MBHloadAddr;

#define	RDFLDR_LENGTH	4096		/* Loader size is 4K */
#define	RDFLDR_DESTLOC	0x100000 	/* and its absolute address */
 
/*
 * Tiny code that moves RDF setup code to its working memory region
 */
unsigned char trampoline_code[] = {
	0xBE, 0, 0, 0, 0,		/* mov	esi,SOURCE_ADDR	*/
	0xBF, 0, 0, 0, 0,		/* mov	edi,DEST_ADDR	*/
	0x89, 0xFA,			/* mov	edx,edi		*/
	0xB9, 0, 4, 0, 0,		/* mov	ecx,RDFLDR_LENGTH/4 */
	0xFC,				/* cld			*/
	0xF3, 0xA5,			/* rep	movsd		*/
	0xFF, 0xE2			/* jmp	edx		*/
};

/* =========================================================================
 * Utility functions
 */


/*
 * initsegments()
 *
 * sets up segments 0, 1, and 2, the initial code data and bss segments
 */

void initsegments()
{
    nsegs = 3;
    outputseg[0].type = 1;
    outputseg[0].number = 0;
    outputseg[0].reserved = 0;
    outputseg[0].length = 0;
    outputseg[1].type = 2;
    outputseg[1].number = 1;
    outputseg[1].reserved = 0;
    outputseg[1].length = 0;
    outputseg[2].type = 0xFFFF;	/* reserved segment type */
    outputseg[2].number = 2;
    outputseg[2].reserved = 0;
    outputseg[2].length = 0;
    bss_length = 0;
}

/*
 * loadmodule
 *
 * Determine the characteristics of a module, and decide what to do with
 * each segment it contains (including determining destination segments and
 * relocation factors for segments that	are kept).
 */

void loadmodule(const char * filename)
{
    if (options.verbose)
	printf("loading `%s'\n", filename);

    /* allocate a new module entry on the end of the modules list */
    if (!modules)
    {
	modules = malloc (sizeof(*modules));
	lastmodule = modules;
    }
    else
    {
	lastmodule->next = malloc (sizeof(*modules));
	lastmodule = lastmodule->next;
    }

    if ( ! lastmodule)
    {
	fprintf(stderr, "ldrdf: out of memory\n");
	exit(1);
    }

    /* open the file using 'rdfopen', which returns nonzero on error */

    if (rdfopen(&lastmodule->f, filename) != 0)
    {
	rdfperror("ldrdf", filename);
	exit(1);
    }

    /* 
     * store information about the module, and determine what segments
     * it contains, and what we should do with them (determine relocation
     * factor if we decide to keep them)
     */

    lastmodule->header = NULL;
    lastmodule->name = strdup(filename);
    lastmodule->next = NULL;

    processmodule(filename, lastmodule);
}
	
/*
 * processmodule()
 *
 * step through each segment, determine what exactly we're doing with
 * it, and if we intend to keep it, determine (a) which segment to
 * put it in and (b) whereabouts in that segment it will end up.
 * (b) is fairly easy, cos we're now keeping track of how big each segment
 * in our output file is...
 */

void processmodule(const char * filename, struct modulenode * mod)
{
    struct segconfig sconf;
    int		seg, outseg;
    void	* header;
    rdfheaderrec * hr;
    long	bssamount = 0;

    for (seg = 0; seg < mod->f.nsegs; seg++)
    {
	/*
	 * get the segment configuration for this type from the segment
	 * table. getsegconfig() is a macro, defined in ldsegs.h.
	 */
	getsegconfig(sconf, mod->f.seg[seg].type);

	if (options.verbose > 1) {
	    printf ("%s %04x [%04x:%10s] ", filename, mod->f.seg[seg].number, 
		    mod->f.seg[seg].type, sconf.typedesc);
	}
	/*
	 * sconf->dowhat tells us what to do with a segment of this type.
	 */
	switch (sconf.dowhat) {
	case SEG_IGNORE:
	    /*
	     * Set destination segment to -1, to indicate that this segment
	     * should be ignored for the purpose of output, ie it is left
	     * out of the linked executable.
	     */
	    mod->seginfo[seg].dest_seg = -1;
	    if (options.verbose > 1) printf("IGNORED\n");
	    break;

	case SEG_NEWSEG:
	    /*
	     * The configuration tells us to create a new segment for
	     * each occurrence of this segment type.
	     */
	    outseg = allocnewseg(sconf.mergetype,
				 mod->f.seg[seg].reserved);
	    mod->seginfo[seg].dest_seg = outseg;
	    mod->seginfo[seg].reloc = 0;
	    outputseg[outseg].length = mod->f.seg[seg].length;
	    if (options.verbose > 1) 
		printf ("=> %04x:%08lx (+%04lx)\n", outseg,
			mod->seginfo[seg].reloc,
			mod->f.seg[seg].length);
	    break;

	case SEG_MERGE:
	    /*
	     * The configuration tells us to merge the segment with
	     * a previously existing segment of type 'sconf.mergetype',
	     * if one exists. Otherwise a new segment is created.
	     * This is handled transparently by 'findsegment()'.
	     */
	    outseg = findsegment(sconf.mergetype,
				 mod->f.seg[seg].reserved);
	    mod->seginfo[seg].dest_seg = outseg;

	    /*
	     * We need to add alignment to these segments.
	     */
	    if (outputseg[outseg].length % options.align != 0)
		outputseg[outseg].length += 
		    options.align - (outputseg[outseg].length % options.align);
	    
	    mod->seginfo[seg].reloc = outputseg[outseg].length;
	    outputseg[outseg].length  += mod->f.seg[seg].length;

	    if (options.verbose > 1) 
		printf ("=> %04x:%08lx (+%04lx)\n", outseg,
			mod->seginfo[seg].reloc,
			mod->f.seg[seg].length);
	}

    }

    /*
     * extract symbols from the header, and dump them into the
     * symbol table
     */
    header = malloc(mod->f.header_len);
    if (!header) {
	fprintf(stderr, "ldrdf: not enough memory\n");
	exit(1);
    }
    if (rdfloadseg(&mod->f, RDOFF_HEADER, header)) {
	rdfperror("ldrdf", filename);
	exit(1);
    }

    while ((hr = rdfgetheaderrec (&mod->f)))
    {
	switch(hr->type) {
	case 2:	/* imported symbol - define with seg = -1 */
	case 7:
	    symtab_add(hr->i.label, -1, 0);
	    break;

	case 3: /* exported symbol */
           {
            int destseg;
            long destreloc;

            if (hr->e.segment == 2)
             {
               destreloc = bss_length;
               if (destreloc % options.align != 0)
                  destreloc +=  options.align - (destreloc % options.align);
               destseg = 2;
             }
            else
             {
               if ((destseg = mod->seginfo[(int)hr->e.segment].dest_seg) == -1)
                  continue;
               destreloc = mod->seginfo[(int)hr->e.segment].reloc;
             }
	    symtab_add(hr->e.label, destseg, destreloc + hr->e.offset);
	    break;
           }

	case 5: /* BSS reservation */
	    /*
	     * first, amalgamate all BSS reservations in this module
	     * into one, because we allow this in the output format.
	     */
	    bssamount += hr->b.amount;
	    break;
	}
    }

    if (bssamount != 0)
    {
	/*
	 * handle the BSS segment - first pad the existing bss length
	 * to the correct alignment, then store the length in bss_reloc
	 * for this module. Then add this module's BSS length onto
	 * bss_length.
	 */
	if (bss_length % options.align != 0)
	    bss_length +=  options.align - (bss_length % options.align);
    
	mod->bss_reloc = bss_length;
	if (options.verbose > 1) {
	    printf ("%s 0002 [            BSS] => 0002:%08lx (+%04lx)\n",
		    filename, bss_length, bssamount);
	}
	bss_length += bssamount;
    }

#ifdef STINGY_MEMORY
    /*
     * we free the header buffer here, to save memory later.
     * this isn't efficient, but probably halves the memory usage
     * of this program...
     */
    mod->f.header_loc = NULL;
    free(header);

#endif

}


/*
 * Look in list for module by its name.
 */
int lookformodule(const char *name)
 {
  struct modulenode *curr=modules;

  while(curr) {
   if (!strcmp(name,curr->name)) return 1;
   curr = curr->next;
  }
  return 0;
 }


/*
 * allocnewseg()
 * findsegment()
 *
 * These functions manipulate the array of output segments, and are used
 * by processmodule(). allocnewseg() allocates a segment in the array,
 * initialising it to be empty. findsegment() first scans the array for
 * a segment of the type requested, and if one isn't found allocates a
 * new one.
 */
int allocnewseg(int16 type,int16 reserved)
{
    outputseg[nsegs].type = type;
    outputseg[nsegs].number = nsegs;
    outputseg[nsegs].reserved = reserved;
    outputseg[nsegs].length = 0;
    outputseg[nsegs].offset = 0;
    outputseg[nsegs].data = NULL;

    return nsegs++;
}

int findsegment(int16 type,int16 reserved)
{
    int i;

    for (i = 0; i < nsegs; i++)
	if (outputseg[i].type == type) return i;

    return allocnewseg(type,reserved);
}

/*
 * symtab_add()
 *
 * inserts a symbol into the global symbol table, which associates symbol
 * names either with addresses, or a marker that the symbol hasn't been
 * resolved yet, or possibly that the symbol has been defined as
 * contained in a dynamic [load time/run time] linked library.
 *
 * segment = -1 => not yet defined
 * segment = -2 => defined as dll symbol
 *
 * If the symbol is already defined, and the new segment >= 0, then
 * if the original segment was < 0 the symbol is redefined, otherwise
 * a duplicate symbol warning is issued. If new segment == -1, this
 * routine won't change a previously existing symbol. It will change
 * to segment = -2 only if the segment was previously < 0.
 */

void symtab_add(const char * symbol, int segment, long offset)
{
    symtabEnt * ste;

    ste = symtabFind(symtab, symbol);
    if (ste)
    {
	if (ste->segment >= 0) {
	    /*
	     * symbol previously defined
	     */
	    if (segment < 0) return;
	    fprintf (error_file, "warning: `%s' redefined\n", symbol);
	    return;
	}

	/*
	 * somebody wanted the symbol, and put an undefined symbol
	 * marker into the table
	 */
	if (segment == -1) return;
	/*
	 * we have more information now - update the symbol's entry
	 */
	ste->segment = segment;
	ste->offset = offset;
	ste->flags = 0;
	return;
    }
    /*
     * this is the first declaration of this symbol
     */
    ste = malloc(sizeof(symtabEnt));
    if (!ste) {
	fprintf(stderr, "ldrdf: out of memory\n");
	exit(1);
    }
    ste->name = strdup(symbol);
    ste->segment = segment;
    ste->offset = offset;
    ste->flags = 0;
    symtabInsert(symtab, ste);
}

/*
 * symtab_get()
 *
 * Retrieves the values associated with a symbol. Undefined symbols
 * are assumed to have -1:0 associated. Returns 1 if the symbol was
 * successfully located.
 */

int symtab_get(const char * symbol, int * segment, long * offset)
{
    symtabEnt * ste = symtabFind(symtab, symbol);
    if (!ste) {
	*segment = -1;
	*offset = 0;
	return 0;
    }
    else
    {
	*segment = ste->segment;
	*offset = ste->offset;
	return 1;
    }
}

/*
 * add_library()
 *
 * checks that a library can be opened and is in the correct format,
 * then adds it to the linked list of libraries.
 */

void add_library(const char * name)
{
    if (rdl_verify(name)) {
	rdl_perror("ldrdf", name);
	errorcount++;
	return;
    }
    if (! libraries)
    {
	lastlib = libraries = malloc(sizeof(*libraries));
	if (! libraries) {
	    fprintf(stderr, "ldrdf: out of memory\n");
	    exit(1);
	}
    }
    else
    {
	lastlib->next = malloc(sizeof(*libraries));
	if (!lastlib->next) {
	    fprintf(stderr, "ldrdf: out of memory\n");
	    exit(1);
	}
	lastlib = lastlib->next;
    }
    lastlib->next = NULL;
    if (rdl_open(lastlib, name)) {
	rdl_perror("ldrdf", name);
	errorcount++;
	return;
    }
}

/*
 * search_libraries()
 *
 * scans through the list of libraries, attempting to match symbols
 * defined in library modules against symbols that are referenced but
 * not defined (segment = -1 in the symbol table)
 *
 * returns 1 if any extra library modules are included, indicating that
 * another pass through the library list should be made (possibly).
 */

int search_libraries()
{
    struct librarynode * cur;
    rdffile f;
    int     i;
    void    * header;
    int	    segment;
    long    offset;
    int	    doneanything = 0, pass = 1, keepfile;
    rdfheaderrec * hr;

    cur = libraries;

    while (cur)
    {
	if (options.verbose > 2)
	    printf("scanning library `%s', pass %d...\n", cur->name, pass);
	
	for (i = 0; rdl_openmodule(cur, i, &f) == 0; i++)
	{
	    if (pass == 2 && lookformodule(f.name)) continue;

	    if (options.verbose > 3)
		printf("  looking in module `%s'\n", f.name);

	    header = malloc(f.header_len);
	    if (!header) {
		fprintf(stderr, "ldrdf: not enough memory\n");
		exit(1);
	    }
	    if (rdfloadseg(&f, RDOFF_HEADER, header)) {
		rdfperror("ldrdf", f.name);
		errorcount++;
		return 0;
	    }
	    
	    keepfile = 0;

	    while ((hr = rdfgetheaderrec (&f)))
	    {
                /* we're only interested in exports, so skip others: */
		if (hr->type != 3) continue; 

		/*
		 * Find the symbol in the symbol table. If the symbol isn't
		 * defined, we aren't interested, so go on to the next.
		 * If it is defined as anything but -1, we're also not
		 * interested. But if it is defined as -1, insert this
		 * module into the list of modules to use, and go
		 * immediately on to the next module...
		 */
		if (! symtab_get(hr->e.label, &segment, &offset) 
		    || segment != -1)
		{
		    continue;    
		}
		
		doneanything = 1;
		keepfile = 1;

		/*
		 * as there are undefined symbols, we can assume that
		 * there are modules on the module list by the time
		 * we get here.
		 */
		lastmodule->next = malloc(sizeof(*lastmodule->next));
		if (!lastmodule->next) {
		    fprintf(stderr, "ldrdf: not enough memory\n");
		    exit(1);
		}
		lastmodule = lastmodule->next;
		memcpy(&lastmodule->f, &f, sizeof(f));
		lastmodule->name = strdup(f.name);
                lastmodule->next = NULL;
		processmodule(f.name, lastmodule);
		break;
	    }
	    if (!keepfile)
             {
              free(f.name);
              f.name = NULL;
              f.fp = NULL;
             }
	}
	if (rdl_error != 0 && rdl_error != RDL_ENOTFOUND)
	    rdl_perror("ldrdf", cur->name);

	cur = cur->next;
	if (cur == NULL && pass == 1) {
	    cur = libraries;
	    pass++;
	}
    }

    return doneanything;
}

/*
 * write_output()
 *
 * this takes the linked list of modules, and walks through it, merging
 * all the modules into a single output module, and then writes this to a
 * file.
 */
void write_output(const char * filename)
{
    FILE          * f;
    rdf_headerbuf * rdfheader;
    struct modulenode * cur;
    int		  i, availableseg, seg, localseg, isrelative;
    void	  * header;
    rdfheaderrec  * hr, newrec;
    symtabEnt 	  * se;
    segtab	  segs;
    long	  offset;
    byte 	  * data;

    if ((f = fopen(filename, "wb"))==NULL) {
	fprintf(stderr, "ldrdf: couldn't open %s for output\n", filename);
	exit(1);
    }
    if ((rdfheader=rdfnewheader())==NULL) {
	fprintf(stderr, "ldrdf: out of memory\n");
	exit(1);
    }
    
    /*
     * Add multiboot header if appropriate option is specified.
     * Multiboot record *MUST* be the first record in output file.
     */
    if (options.addMBheader) {
	if (options.verbose)
	    puts("\nadding multiboot header record");
    
        hr = (rdfheaderrec *) malloc(sizeof(struct MultiBootHdrRec));
        hr->mbh.type = 9;
        hr->mbh.reclen = sizeof(struct tMultiBootHeader)+TRAMPOLINESIZE;
	    
        hr->mbh.mb.Magic = MB_MAGIC;
        hr->mbh.mb.Flags = MB_FL_KLUDGE;
	hr->mbh.mb.Checksum = ~(MB_MAGIC+MB_FL_KLUDGE-1);
        hr->mbh.mb.HeaderAddr = MBHloadAddr+16;
        hr->mbh.mb.LoadAddr = MBHloadAddr;
	hr->mbh.mb.Entry = MBHloadAddr+16+sizeof(struct tMultiBootHeader);
	
	memcpy(hr->mbh.trampoline,trampoline_code,TRAMPOLINESIZE);
	    
        rdfaddheader(rdfheader,hr);
        free(hr);
    }

    if (options.verbose)
	printf ("\nbuilding output module (%d segments)\n", nsegs);

    /*
     * Allocate the memory for the segments. We may be better off
     * building the output module one segment at a time when running
     * under 16 bit DOS, but that would be a slower way of doing this.
     * And you could always use DJGPP...
     */
    for (i = 0; i < nsegs; i++)
    {
	outputseg[i].data=NULL;
	if(!outputseg[i].length) continue;
	outputseg[i].data = malloc(outputseg[i].length);
	if (!outputseg[i].data) {
	    fprintf(stderr, "ldrdf: out of memory\n");
	    exit(1);
	}
    }

    /*
     * initialise availableseg, used to allocate segment numbers for
     * imported and exported labels...
     */
    availableseg = nsegs;

    /*
     * Step through the modules, performing required actions on each one
     */
    for (cur = modules; cur; cur=cur->next)
    {
	/*
	 * Read the actual segment contents into the correct places in
	 * the newly allocated segments
	 */

	for (i = 0; i < cur->f.nsegs; i++)
	{
	    int dest = cur->seginfo[i].dest_seg;

	    if (dest == -1) continue;
	    if (rdfloadseg(&cur->f, i, 
			   outputseg[dest].data + cur->seginfo[i].reloc))
	    {
		rdfperror("ldrdf", cur->name);
		exit(1);
	    }
	}
 
	/*
	 * Perform fixups, and add new header records where required
	 */

	header = malloc(cur->f.header_len);
	if (!header) {
	    fprintf(stderr, "ldrdf: out of memory\n");
	    exit(1);
	}

	if (cur->f.header_loc)
	    rdfheaderrewind(&cur->f);
	else
	    if (rdfloadseg(&cur->f, RDOFF_HEADER, header))
	    {
		rdfperror("ldrdf", cur->name);
		exit(1);
	    }
	
	/*
	 * we need to create a local segment number -> location
	 * table for the segments in this module.
	 */
	init_seglocations(&segs);
	for (i = 0; i < cur->f.nsegs; i++)
	{
	    add_seglocation(&segs, cur->f.seg[i].number,
			    cur->seginfo[i].dest_seg, cur->seginfo[i].reloc);
	}
	/*
	 * and the BSS segment (doh!)
	 */
	add_seglocation (&segs, 2, 2, cur->bss_reloc);
	
	while ((hr = rdfgetheaderrec(&cur->f)))
	{
	    switch(hr->type) {
	    case 1: /* relocation record - need to do a fixup */
		/*
		 * First correct the offset stored in the segment from
		 * the start of the segment (which may well have changed).
		 *
		 * To do this we add to the number stored the relocation
		 * factor associated with the segment that contains the
		 * target segment.
		 *
		 * The relocation could be a relative relocation, in which
		 * case we have to first subtract the amount we've relocated
		 * the containing segment by.
		 */
		
		if (!get_seglocation(&segs, hr->r.refseg, &seg, &offset))
		{
		    fprintf(stderr, "%s: reloc to undefined segment %04x\n",
			    cur->name, (int) hr->r.refseg);
		    errorcount++;
		    break;
		}

		isrelative = (hr->r.segment & 64) == 64;
		hr->r.segment &= 63;

		if (hr->r.segment == 2 || 
		    (localseg = rdffindsegment(&cur->f, hr->r.segment)) == -1)
		{
		    fprintf(stderr, "%s: reloc from %s segment (%d)\n", 
			    cur->name,
			    hr->r.segment == 2 ? "BSS" : "unknown",
			    hr->r.segment);
		    errorcount++;
		    break;
		}

		if (hr->r.length != 1 && hr->r.length != 2 && hr->r.length!=4)
		{
		    fprintf(stderr, "%s: nonstandard length reloc "
			    "(%d bytes)\n", cur->name, hr->r.length);
		    errorcount++;
		    break;
		}

		/* 
		 * okay, now the relocation is in the segment pointed to by
		 * cur->seginfo[localseg], and we know everything else is
		 * okay to go ahead and do the relocation
		 */
		data = outputseg[cur->seginfo[localseg].dest_seg].data;
		data += cur->seginfo[localseg].reloc + hr->r.offset;

		/*
		 * data now points to the reference that needs
		 * relocation. Calculate the relocation factor.
		 * Factor is:
		 *      offset of referred object in segment [in offset]
		 *	(- relocation of localseg, if ref is relative)
		 * For simplicity, the result is stored in 'offset'.
		 * Then add 'offset' onto the value at data.
		 */
		
		if (isrelative) offset -= cur->seginfo[localseg].reloc;
		switch (hr->r.length)
		{
		case 1:
		    offset += *data;
		    if (offset < -127 || offset > 128)
			fprintf(error_file, "warning: relocation out of range "
				"at %s(%02x:%08lx)\n", cur->name,
				(int)hr->r.segment, hr->r.offset);
		    *data = (char) offset;
		    break;
		case 2:
		    offset += * (short *)data;
		    if (offset < -32767 || offset > 32768)
			fprintf(error_file, "warning: relocation out of range "
				"at %s(%02x:%08lx)\n", cur->name,
				(int)hr->r.segment, hr->r.offset);
		    * (short *)data = (short) offset;
		    break;
		case 4:
		    * (long *)data += offset;
		    /* we can't easily detect overflow on this one */
		    break;
		}

		/*
		 * If the relocation was relative between two symbols in
		 * the same segment, then we're done.
		 *
		 * Otherwise, we need to output a new relocation record
		 * with the references updated segment and offset...
		 */
		 
		if (isrelative && cur->seginfo[localseg].dest_seg != seg)
		{
		    hr->r.segment = cur->seginfo[localseg].dest_seg+64;
		    hr->r.offset += cur->seginfo[localseg].reloc;
		    hr->r.refseg = seg;
		    rdfaddheader(rdfheader, hr);
		    break;
		}
		 
		if (! isrelative || cur->seginfo[localseg].dest_seg != seg)
		{
		    hr->r.segment = cur->seginfo[localseg].dest_seg;
		    hr->r.offset += cur->seginfo[localseg].reloc;
		    hr->r.refseg = seg;
		    rdfaddheader(rdfheader, hr);
		}
		break;

	    case 2: /* import symbol */
	    case 7:
		/*
		 * scan the global symbol table for the symbol
		 * and associate its location with the segment number 
		 * for this module
		 */
		se = symtabFind(symtab, hr->i.label);
		if (!se || se->segment == -1) {
		    if (options.warnUnresolved) {
			fprintf(error_file, "warning: unresolved reference to `%s'"
				" in module `%s'\n", hr->i.label, cur->name);
			if (options.errorUnresolved==1) errorcount++;
		    }
		    /*
		     * we need to allocate a segment number for this
		     * symbol, and store it in the symbol table for
		     * future reference
		     */ 
		    if (!se) {
			se=malloc(sizeof(*se));
			if (!se) {
			    fprintf(stderr, "ldrdf: out of memory\n");
			    exit(1);
			}
			se->name = strdup(hr->i.label);
			se->flags = 0;
			se->segment = availableseg++;
			se->offset = 0;
			symtabInsert(symtab, se);
		    }
		    else {
			se->segment = availableseg++;
			se->offset = 0;
		    }
		    /*
		     * output a header record that imports it to the
		     * recently allocated segment number...
		     */
		    newrec = *hr;
		    newrec.i.segment = se->segment;
		    rdfaddheader(rdfheader, &newrec);
		}

		add_seglocation(&segs, hr->i.segment, se->segment, se->offset);
		
		break;

	    case 3: /* export symbol */
		/*
		 * need to insert an export for this symbol into the new
		 * header, unless we're stripping symbols. Even if we're
		 * stripping, put the symbol if it's marked as SYM_GLOBAL.
		 */
		if (options.strip && !(hr->e.flags & SYM_GLOBAL))
		    break;

		if (hr->e.segment == 2) {
		    seg = 2;
		    offset = cur->bss_reloc;
		}
		else {
		    localseg = rdffindsegment(&cur->f, hr->e.segment);
		    if (localseg == -1) {
			fprintf(stderr, "%s: exported symbol `%s' from "
				"unrecognised segment\n", cur->name,
				hr->e.label);
			errorcount++;
			break;
		    }
		    offset = cur->seginfo[localseg].reloc;
		    seg = cur->seginfo[localseg].dest_seg;
		}

		hr->e.segment = seg;	
		hr->e.offset += offset;
		rdfaddheader(rdfheader, hr);
		break;

	    case 8: /* module name */
		 /*
		  * Insert module name record if export symbols
		  * are not stripped.
		  * If module name begins with '$' - insert it anyway.
		  */
		  
		if (options.strip && hr->m.modname[0] != '$') break;

		rdfaddheader(rdfheader, hr);
		break;

	    case 6: /* segment fixup */
		/*
		 * modify the segment numbers if necessary, and
		 * pass straight through to the output module header
		 *
		 * *** FIXME ***
		 */
		if (hr->r.segment == 2) {
		    fprintf(stderr, "%s: segment fixup in BSS section\n",
			    cur->name);
		    errorcount++;
		    break;
		}
		localseg = rdffindsegment(&cur->f, hr->r.segment);
		if (localseg == -1) {
		    fprintf(stderr, "%s: segment fixup in unrecognised"
			    " segment (%d)\n", cur->name, hr->r.segment);
		    errorcount++;
		    break;
		}
		hr->r.segment = cur->seginfo[localseg].dest_seg;
		hr->r.offset += cur->seginfo[localseg].reloc;

		if (!get_seglocation(&segs, hr->r.refseg, &seg, &offset))
		{
		    fprintf(stderr, "%s: segment fixup to undefined "
			    "segment %04x\n", cur->name, (int)hr->r.refseg);
		    errorcount++;
		    break;
		}
		hr->r.refseg = seg;
		rdfaddheader(rdfheader, hr);
		break;
	    }
	}

	free(header);
	done_seglocations(&segs);

    }

    /*
     * combined BSS reservation for the entire results
     */
    newrec.type = 5;
    newrec.b.reclen = 4;
    newrec.b.amount = bss_length;
    rdfaddheader(rdfheader, &newrec);

    /*
     * Write the header
     */
    for (i = 0; i < nsegs; i++)
    {
	if (i == 2) continue;
	rdfaddsegment (rdfheader, outputseg[i].length);
    }
    
    if (options.addMBheader) {
	struct MultiBootHdrRec *mbhrec = (struct MultiBootHdrRec *)(rdfheader->buf->buffer);
	unsigned l = membuflength(rdfheader->buf) + 14 + 
		     10*rdfheader->nsegments + rdfheader->seglength;
	unsigned *ldraddr = (unsigned *)(mbhrec->trampoline+1);
	unsigned *ldrdest = (unsigned *)(mbhrec->trampoline+6);

	mbhrec->mb.LoadEndAddr = MBHloadAddr+l+10+RDFLDR_LENGTH; 
	mbhrec->mb.BSSendAddr = mbhrec->mb.LoadEndAddr;
	
	*ldraddr = MBHloadAddr+l+10;
	*ldrdest = RDFLDR_DESTLOC;
    }
    
    rdfwriteheader(f, rdfheader);
    rdfdoneheader(rdfheader);
    /*
     * Step through the segments, one at a time, writing out into
     * the output file
     */
    
    for (i = 0; i < nsegs; i++)
    {
	int16 s;
	long l;
	
	if (i == 2) continue;

	s = translateshort(outputseg[i].type);
	fwrite(&s, 2, 1, f);
	s = translateshort(outputseg[i].number);
	fwrite(&s, 2, 1, f);
	s = translateshort(outputseg[i].reserved);
	fwrite(&s, 2, 1, f);
	l = translatelong(outputseg[i].length);
	fwrite(&l, 4, 1, f);

	fwrite(outputseg[i].data, outputseg[i].length, 1, f);
    }

    fwrite("\0\0\0\0\0\0\0\0\0\0", 10, 1, f);
}

/* =========================================================================
 * Main program
 */

void usage()
{
    printf("usage:\n");
    printf("   ldrdf [options] object modules ... [-llibrary ...]\n");
    printf("   ldrdf -r\n");
    printf("options:\n");
    printf("   -v[=n]          increases verbosity by 1, or sets it to n\n");
    printf("   -a nn           sets segment alignment value (default 16)\n");
    printf("   -s              strips exported symbols\n");
    printf("   -x              warn about unresolved symbols\n");
    printf("   -o name         write output in file 'name'\n");
    printf("   -j path         specify objects search path\n");
    printf("   -L path         specify libraries search path\n");
    printf("   -mbh [address]  add multiboot header to output file. Default\n");
    printf("                   loading address is 0x110000\n");
    exit(0);
}

int main(int argc, char ** argv)
{
    char * outname = "aout.rdf";
    int  moduleloaded = 0;
    char *respstrings[128] = {0, };

    options.verbose = 0;
    options.align = 16;
    options.warnUnresolved = 0;
    options.strip = 0;

    error_file = stderr;

    argc --, argv ++;
    if (argc == 0) usage();
    while (argc && **argv == '-' && argv[0][1] != 'l')
    {
	switch(argv[0][1]) {
	case 'r':
	    printf("ldrdf (linker for RDF files) version " LDRDF_VERSION "\n");
	    printf( _RDOFF_H "\n");
	    exit(0);
	case 'v':
	    if (argv[0][2] == '=') {
		options.verbose = argv[0][3] - '0';
		if (options.verbose < 0 || options.verbose > 9) {
		    fprintf(stderr, "ldrdf: verbosity level must be a number"
			    " between 0 and 9\n");
		    exit(1);
		}
	    }
	    else
		options.verbose++;
	    break;
	case 'a':
	    options.align = atoi(argv[1]);
	    if (options.align <= 0) {
		fprintf(stderr,
			"ldrdf: -a expects a positive number argument\n");
		exit(1);
	    }
	    argv++, argc--;
	    break;
	case 's':
	    options.strip = 1;
	    break;
	case 'x':
	    options.warnUnresolved = 1;
	    if (argv[0][2]=='e')
		options.errorUnresolved = 1;
	    break;
	case 'o':
	    outname = argv[1];
	    argv++, argc--;
	    break;
	case 'j':
	    if (!objpath)
	     {
              options.objpath = 1;
	      objpath = argv[1];
	      argv++, argc--;
	      break;
	     }
	    else
	     {
	      fprintf(stderr,"ldrdf: more than one objects search path specified\n");
	      exit(1);
	     }
	case 'L':
	    if (!libpath)
             {
              options.libpath = 1;
	      libpath = argv[1];
	      argv++, argc--;
	      break;
	     }
	    else
	     {
	      fprintf(stderr,"ldrdf: more than one libraries search path specified\n");
	      exit(1);
	     }
	case '@': {
	      int i=0;
	      char buf[256];
	      FILE *f;

              options.respfile = 1;
	      if (argv[1] != NULL) f = fopen(argv[1],"r");
	      else
	       {
		fprintf(stderr,"ldrdf: no response file name specified\n");
		exit(1);
	       }

	      if (f == NULL)
	       {
		fprintf(stderr,"ldrdf: unable to open response file\n");
		exit(1);
	       }
	      argc-=2;
	      while(fgets(buf,sizeof(buf)-1,f)!=NULL)
	       {
		char *p;
		if (buf[0]=='\n') continue;
		if ((p = strchr(buf,'\n')) != 0)
		 *p=0;
		if (i >= 128)
		 {
		  fprintf(stderr,"ldrdf: too many input files\n");
		  exit(1);
		 }
		*(respstrings+i) = newstr(buf);
		argc++, i++;
	       }
	      goto done;
	 }
	case '2':
            options.stderr_redir = 1;
	    error_file = stdout;
	    break;
	case 'm':
	    if (argv[0][2] == 'b' && argv[0][3] == 'h') {
		if (argv[1][0] != '-') {
		    MBHloadAddr = atoi(argv[1]);
		} else {
		    MBHloadAddr = MB_DEFAULTLOADADDR;
		}        
        	options.addMBheader = 1;
		break;    
	    }		
	default:
	    usage();
	}
	argv++, argc--;
    }
done:
    if (options.verbose > 4) {
	printf("ldrdf invoked with options:\n");
	printf("    section alignment: %d bytes\n", options.align);
	printf("    output name: `%s'\n", outname);
	if (options.strip)
	    printf("    strip symbols\n");
	if (options.warnUnresolved)
	    printf("    warn about unresolved symbols\n");
	if (options.errorUnresolved)
	    printf("    error if unresolved symbols\n");    
        if (options.objpath)
            printf("    objects search path: %s\n",objpath);
        if (options.libpath)
            printf("    libraries search path: %s\n",libpath);
	if (options.addMBheader)
            printf("    loading address for multiboot header: 0x%X\n",MBHloadAddr);    
	printf("\n");
    }

    symtab = symtabNew();
    initsegments();

    if (!symtab) {
	fprintf(stderr, "ldrdf: out of memory\n");
	exit(1);
    }

    if (*respstrings) argv = respstrings;
    while (argc)
    {
	if (!strncmp(*argv, "-l", 2)) /* library */
         {
	  if(libpath) add_library(newstrcat(libpath,*argv + 2));
          else add_library(*argv + 2);
         }
	else {
	    if(objpath) loadmodule(newstrcat(objpath,*argv));
	    else loadmodule(*argv);
	    moduleloaded = 1;
	}
	argv++, argc--;
    }

    if (! moduleloaded) {
	printf("ldrdf: nothing to do. ldrdf -h for usage\n");
	return 0;
    }


    search_libraries();

    if (options.verbose > 2)
    {
	printf ("symbol table:\n");
	symtabDump(symtab, stdout);
    }

    write_output(outname);

    if (errorcount > 0)	exit(1);
    return 0;
}
