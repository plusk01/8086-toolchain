/* rdlib.h	Functions for manipulating librarys of RDOFF object files */


struct librarynode {
    char	* name;
    FILE	* fp;		/* initialised to NULL - always check*/
    int		referenced;	/* & open if required. Close afterwards */
    struct librarynode * next;  /* if ! referenced. */
};


extern int rdl_error;

#define RDL_EOPEN     1
#define RDL_EINVALID  2
#define RDL_EVERSION  3
#define RDL_ENOTFOUND 4

int rdl_verify (const char * filename);
int rdl_open (struct librarynode * lib, const char * filename);
int rdl_searchlib (struct librarynode * lib,
		   const char * label, rdffile * f);
int rdl_openmodule (struct librarynode * lib, int module, rdffile * f);

void rdl_perror(const char *apname, const char *filename);


