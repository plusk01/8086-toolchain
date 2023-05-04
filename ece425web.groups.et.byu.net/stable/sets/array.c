#include "clib.h"

#define s0 "a short string"
#define s1 "a very, very, VERY long string, yea, exceeding all others"
#define s2 "a nice string of medium length"
#define s3 "a shortish string"

char a[][58] = { s0, s1, s2, s3 };
char *b[]    = { s0, s1, s2, s3 };

int mystrlen(char *s)
{   /* returns length of string s */
    int i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

void main(void)
{
    int i;
    printString("Contents of a[][]\n");
    for (i = 0; i < 4; i++)
    {
        printString(" <");
        printInt(mystrlen(a[i]));
        printString("> ");
        printString(a[i]);
        printNewLine();
    }
    printString("Contents of *b[]\n");
    for (i = 0; i < 4; i++)
    {
        printString(" <");
        printInt(mystrlen(b[i]));
        printString("> ");
        printString(b[i]);
        printNewLine();
    }
}

