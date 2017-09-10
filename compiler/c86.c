/*
**	This file generates the message resource file for use on the
**	Psion s3a PDA.
*/

#include "config.h"

STRUCT STRING
    {
	TEXT	str;
    }

#define	MSG(x,y,z)	RESOURCE STRING y { str = z; }
#include "message.h"
