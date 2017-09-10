#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdoff.h"
#include "rdlib.h"

/*
 * format of RDOFF library files:
 * optional signature ('.sig')
 * repeat
 *   null terminated module name (max 255 chars)
 *   RDOFF module
 * until eof
 * optional directory ('.dir')
 */

/*
 * TODO
 *
 * No support exists yet for special modules. But we aren't using
 * any special modules yet. They are only defined now so that their
 * existance doesn't break older versions of the linker... presently
 * anything whose name begins with '.' is ignored.
 */

int rdl_error = 0;

char *rdl_errors[5] = {
    "no error","could not open file", "invalid file structure",
    "file contains modules of an unsupported RDOFF version",
    "module not found"
};
int rdl_verify(const char * filename)
{
    FILE    * fp = fopen(filename, "rb");
    char    buf[257];
    int     i;
    long    length;
    static char lastverified[256];
    static int lastresult = -1;

    if (lastresult != -1 && !strcmp(filename, lastverified))
	return lastresult;

    strcpy(lastverified, filename);

    if (!fp)
	return (rdl_error = lastresult = 1);
    
    while (!feof(fp))
    {
	i = 0;

	while (fread(buf + i,1,1,fp) == 1 && buf[i] && i < 257)
	    i++;
	if (feof(fp)) break;
	
	if (buf[0] == '.') {
	    /*
	     * A special module, eg a signature block or a directory.
	     * Format of such a module is defined to be:
	     *   six char type identifier
	     *   long count bytes content
	     *   content
	     * so we can handle it uniformaly with RDOFF2 modules.
	     */
	     fread(buf, 6, 1, fp);
	     buf[6] = 0;
	     /* Currently, nothing useful to do with signature block.. */
	} else {
	    fread(buf, 6, 1, fp);
	    buf[6] = 0;
	    if (strncmp(buf, "RDOFF", 5)) {
		return rdl_error = lastresult = 2;
	    } else if (buf[5] != '2') {
		return rdl_error = lastresult = 3;
	    }
	}
	fread(&length, 4, 1, fp);
	fseek(fp, length, SEEK_CUR); /* skip over the module */
    }
    fclose(fp);
    return lastresult = 0;	     /* library in correct format */
}

int rdl_open (struct librarynode * lib, const char * name)
{
    int i = rdl_verify(name);
    if (i) return i;

    lib->fp = NULL;
    lib->name = strdup(name);
    lib->referenced = 0;
    lib->next = NULL;
    return 0;
}

void rdl_close (struct librarynode * lib)
{
    if (lib->fp)
	fclose(lib->fp);
    free(lib->name);
}

int rdl_searchlib (struct librarynode * lib,
		   const char * label, rdffile * f)
{
    char		buf[512];
    int			i, t;
    void *		hdr;
    rdfheaderrec	* r;
    long		l;

    rdl_error = 0;
    lib->referenced ++;

    if (! lib->fp)
    {
	lib->fp = fopen(lib->name,"rb");
	
	if (! lib->fp) {
	    rdl_error = 1;
	    return 0;
	}
    }
    else
	rewind(lib->fp);

    while (! feof(lib->fp) )
    {
	/* 
	 * read the module name from the file, and prepend 
	 * the library name and '.' to it.
	 */
	strcpy(buf, lib->name);

	i = strlen(lib->name);
	buf[i++] = '.'; t = i;
	while (fread(buf + i,1,1,lib->fp) == 1 && buf[i] && i < 512)
	    i++;

	buf[i] = 0;
	
	if (feof(lib->fp)) break;
	if (!strcmp(buf + t, ".dir")) 	/* skip over directory */
	{
	    fread (&l, 4, 1, lib->fp);
	    fseek (lib->fp, l, SEEK_CUR);
	    continue;
	}
	/*
	 * open the RDOFF module
	 */
	if ( rdfopenhere(f,lib->fp,&lib->referenced,buf) ) {
	    rdl_error = 16 * rdf_errno;
	    return 0;
	}
	/*
	 * read in the header, and scan for exported symbols
	 */
	hdr = malloc(f->header_len);
	rdfloadseg(f,RDOFF_HEADER,hdr);
	
	while ((r = rdfgetheaderrec(f)))
	{
	    if (r->type != 3)	/* not an export */
		continue;

	    if (! strcmp(r->e.label, label) )	/* match! */
	    {
		free(hdr);			/* reset to 'just open' */
		f->header_loc = NULL;		/* state... */
		f->header_fp = 0;
		return 1;
	    }
	}

	/* find start of next module... */
	i = f->eof_offset;
	rdfclose(f);
	fseek(lib->fp,i,SEEK_SET);
    }

    /*
     * close the file if nobody else is using it
     */
    lib->referenced --;
    if (! lib->referenced)
    {
	fclose(lib->fp);
	lib->fp = NULL;
    }
    return 0;
}

int rdl_openmodule (struct librarynode * lib, int moduleno, rdffile * f)
{
    char    buf[512];
    int     i, cmod, t;
    long    length;

    lib->referenced++;

    if (!lib->fp)
    {
	lib->fp = fopen(lib->name, "rb");
	if (!lib->fp) {
	    lib->referenced--;
	    return (rdl_error = 1);   
	}
    }
    else
	rewind(lib->fp);

    cmod = -1;
    while (!feof(lib->fp))
    {
	strcpy(buf, lib->name);
	i = strlen(buf); 
	buf[i++] = '.'; t = i;
	while (fread(buf + i,1,1,lib->fp) == 1 && buf[i] && i < 512)
	    i++;
	buf[i] = 0;
	if (feof(lib->fp)) break;

	if (buf[t] != '.') /* special module - not counted in the numbering */
	    cmod++;	   /* of RDOFF modules - must be referred to by name */

	if (cmod == moduleno) {
	    rdl_error = 16 *
		rdfopenhere(f, lib->fp, &lib->referenced, buf);
	    lib->referenced--;
	    if (!lib->referenced) {
		fclose(lib->fp);
		lib->fp = NULL;
	    }
	    return rdl_error;
	}

	fread(buf, 6, 1, lib->fp);
	buf[6] = 0;
	if (buf[t] == '.') {
	    /* do nothing */
	}
	else if (strncmp(buf, "RDOFF", 5)) {
	    if (! --lib->referenced) {
		fclose(lib->fp);
		lib->fp = NULL;
	    }
	    return rdl_error = 2;
	}
	else if (buf[5] != '2') {
	    if (! --lib->referenced) {
		fclose(lib->fp);
		lib->fp = NULL;
	    }
	    return rdl_error = 3;
	}

	fread(&length, 4, 1, lib->fp);
	fseek(lib->fp, length, SEEK_CUR); /* skip over the module */
    }
    if (! --lib->referenced) {
	fclose(lib->fp);
	lib->fp = NULL;
    }
    return rdl_error = 4;	/* module not found */
}

void rdl_perror(const char *apname, const char *filename)
{
    if (rdl_error >= 16)
	rdfperror(apname, filename);
    else
	fprintf(stderr,"%s:%s:%s\n",apname,filename,rdl_errors[rdl_error]);
}



