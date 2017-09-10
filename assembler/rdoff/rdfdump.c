#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdoff.h"
#include "multboot.h"

FILE *infile;

long translatelong(long in) {		/* translate from little endian to
					   local representation */
  long r;
  unsigned char *i;

  i = (unsigned char *)&in;
  r = i[3];
  r = (r << 8) + i[2];
  r = (r << 8) + i[1];
  r = (r << 8) + *i;

  return r;
}
  
int16 translateshort(int16 in) {
  int r;
  unsigned char *i;

  i = (unsigned char *)&in;
  r = (i[1] << 8) + *i;

  return r;
}

void print_header(long length, int rdf_version) {
  char buf[129],t,s,l,flags;
  unsigned char reclen;
  long o,ll;
  int16 rs;
  struct tMultiBootHeader *mb;

  while (length > 0) {
    fread(&t,1,1,infile);
    if (rdf_version >= 2) {
	fread(&reclen,1,1,infile);
    }
    switch(t) {
    
    case 1:		/* relocation record */
    case 6:		/* segment relocation */
      fread(&s,1,1,infile);
      fread(&o,4,1,infile);
      fread(&l,1,1,infile);
      fread(&rs,2,1,infile);
      printf("  %s: location (%04x:%08lx), length %d, "
	     "referred seg %04x\n", t == 1 ? "relocation" : "seg relocation",
	     (int)s,translatelong(o),(int)l,
	     translateshort(rs));
      if (rdf_version >= 2 && reclen != 8)
	  printf("    warning: reclen != 8\n");
      if (rdf_version == 1) length -= 9;
      if (rdf_version == 1 && t == 6)
	  printf("    warning: seg relocation not supported in RDOFF1\n");
      break;
      
    case 2:             /* import record */
    case 7:		/* import far symbol */
      fread(&rs,2,1,infile);
      ll = 0;

      if (rdf_version == 1) {
	  do {
	      fread(&buf[ll],1,1,infile);
	  } while (buf[ll++]);
      }
      else
      {
	  for (;ll < reclen - 2; ll++)
	      fread(&buf[ll],1,1,infile);
      }

      printf("  %simport: segment %04x = %s\n",t == 7 ? "far " : "",
	     translateshort(rs),buf);
      if (rdf_version == 1) length -= ll + 3;
      if (rdf_version == 1 && t == 7)
	  printf ("    warning: far import not supported in RDOFF1\n");
      break;
      
    case 3:             /* export record */
      fread(&flags,1,1,infile);
      fread(&s,1,1,infile);
      fread(&o,4,1,infile);
      ll = 0;

      if (rdf_version == 1) {
	  do {
	      fread(&buf[ll],1,1,infile);
	  } while (buf[ll++]);
      }
      else
      {
	  for (; ll < reclen - 6; ll ++)
	      fread(&buf[ll],1,1,infile);
      }
      if (flags & SYM_GLOBAL)
        printf("  export");
      else
        printf("  global");
      if (flags & SYM_FUNCTION) printf(" proc");
      if (flags & SYM_DATA) printf(" data");
      printf(": (%04x:%08lx) = %s\n",(int)s,translatelong(o),buf);
      if (rdf_version == 1) length -= ll + 6;
      break;
      
    case 4:		/* DLL and Module records */
    case 8:
      ll = 0;

      if (rdf_version == 1) {
	  do {
	      fread(&buf[ll],1,1,infile);
	  } while (buf[ll++]);
      }
      else
      {
	  for (; ll < reclen; ll++)
	      fread(&buf[ll],1,1,infile);
      }
      if (t==4) printf("  dll: %s\n",buf);
      else printf("  module: %s\n",buf);
      if (rdf_version == 1) length -= ll + 1;
      break;
    case 5:		/* BSS reservation */
      fread(&ll,4,1,infile);
      printf("  bss reservation: %08lx bytes\n",translatelong(ll));
      if (rdf_version == 1) length -= 5;
      if (rdf_version > 1 && reclen != 4)
	  printf("    warning: reclen != 4\n");
      break;
      
    case 9:		/* MultiBoot header record */
      fread(buf,reclen,1,infile);
      mb = (struct tMultiBootHeader *)buf;
      printf("  multiboot header: load address=0x%X, size=0x%X, entry=0x%X\n",
             mb->LoadAddr, mb->LoadEndAddr - mb->LoadAddr, mb->Entry);  
      break;  
    default:
      printf("  unrecognised record (type %d",(int)t);
      if (rdf_version > 1) {
      	printf(", length %d",(int)reclen);
	fseek(infile,reclen,SEEK_CUR);
      }		
      printf(")\n");
      if (rdf_version == 1) length --;
    }
    if (rdf_version != 1) length -= 2 + reclen;
  }
}

char * knowntypes[8] = {"NULL", "text", "data", "object comment",
			"linked comment", "loader comment",
			"symbolic debug", "line number debug"};

char * translatesegmenttype(int16 type) {
    if (type < 8) return knowntypes[type];
    if (type < 0x0020) return "reserved";
    if (type < 0x1000) return "reserved - moscow";
    if (type < 0x8000) return "reserved - system dependant";
    if (type < 0xFFFF) return "reserved - other";
    if (type == 0xFFFF) return "invalid type code";
    return "type code out of range";
}    

int main(int argc,char **argv) {
  char id[7];
  long l;
  int16 s;
  int verbose = 0;
  long offset;
  int  foundnullsegment = 0;
  int  version;
  long segmentcontentlength = 0;
  int  nsegments = 0;
  long headerlength = 0;
  long objectlength = 0;

  puts("RDOFF Dump utility v2.1\n(c) Copyright 1996,99,2000 Julian R Hall, Yuri M Zaporogets");

  if (argc < 2) {
    fputs("Usage: rdfdump [-v] <filename>\n",stderr);
    exit(1);
  }

  if (! strcmp (argv[1], "-v") )
  {
    verbose = 1;
    if (argc < 3)
    {
      fputs("required parameter missing\n",stderr);
      exit(1);
    }
    argv++;
  }

  infile = fopen(argv[1],"rb");
  if (! infile) {
    fprintf(stderr,"rdfdump: Could not open %s\n",argv[1]);
    exit(1);
  }

  fread(id,6,1,infile);
  if (strncmp(id,"RDOFF",5)) {
    fputs("rdfdump: File does not contain valid RDOFF header\n",stderr);
    exit(1);
  }

  printf("File %s: RDOFF version %c\n\n",argv[1],id[5]);
  if (id[5] < '1' || id[5] > '2') {
    fprintf(stderr,"rdfdump: unknown RDOFF version '%c'\n",id[5]);
    exit(1);
  }
  version = id[5] - '0';

  if (version > 1) {
      fread(&l, 4, 1, infile);
      objectlength = translatelong(l);
      printf("Object content size: %ld bytes\n", objectlength);
  }

  fread(&l,4,1,infile);
  headerlength = translatelong(l);
  printf("Header (%ld bytes):\n",headerlength);
  print_header(headerlength, version);

  if (version == 1) {
      fread(&l,4,1,infile);
      l = translatelong(l);
      printf("\nText segment length = %ld bytes\n",l);
      offset = 0;
      while(l--) {
	  fread(id,1,1,infile);
	  if (verbose) {
	      if (offset % 16 == 0)
		  printf("\n%08lx ", offset);
	      printf(" %02x",(int) (unsigned char)id[0]);
	      offset++;
	  }
      }
      if (verbose) printf("\n\n");
      
      fread(&l,4,1,infile);
      l = translatelong(l);
      printf("Data segment length = %ld bytes\n",l);

      if (verbose)
      {
	  offset = 0;
	  while (l--) {
	      fread(id,1,1,infile);
	      if (offset % 16 == 0)
		  printf("\n%08lx ", offset);
	      printf(" %02x",(int) (unsigned char) id[0]);
	      offset++;
	  }
	  printf("\n");
      }
  }
  else
  {
      do {
	  fread(&s,2,1,infile);
	  s = translateshort(s);
	  if (!s) {
	      printf("\nNULL segment\n");
	      foundnullsegment = 1;
	      break;
	  }
	  printf("\nSegment:\n  Type   = %04X (%s)\n",(int)s,
		 translatesegmenttype(s));
	  nsegments++;

	  fread(&s,2,1,infile);
	  printf("  Number = %04X\n",(int)translateshort(s));
	  fread(&s,2,1,infile);
	  printf("  Resrvd = %04X\n",(int)translateshort(s));
	  fread(&l,4,1,infile);
	  l = translatelong(l);
	  printf("  Length = %ld bytes\n",l);
	  segmentcontentlength += l;

	  offset = 0;
	  while(l--) {
	      fread(id,1,1,infile);
	      if (verbose) {
		  if (offset % 16 == 0)
		      printf("\n%08lx ", offset);
		  printf(" %02x",(int) (unsigned char)id[0]);
		  offset++;
	      }
	  }
	  if (verbose) printf("\n");
      } while (!feof(infile));
      if (! foundnullsegment)
	  printf("\nWarning: unexpected end of file - "
		 "NULL segment not found\n");

      printf("\nTotal number of segments: %d\n", nsegments);
      printf("Total segment content length: %ld bytes\n",segmentcontentlength);

      /* calculate what the total object content length should have been */
      l = segmentcontentlength + 10 * (nsegments+1) + headerlength + 4;
      if (l != objectlength)
	  printf("Warning: actual object length (%ld) != "
		 "stored object length (%ld)\n", l, objectlength);
  }
  fclose(infile);
  return 0;
}
