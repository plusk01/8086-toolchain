/* Last updated 2 October 2003 */

#include "clib.h"

int k;

void a(void)
{
    static int i = 0;
    int j = 0;
    
    i++;
    j++;
    printString("in a(), i = ");
    printInt(i);
    printString(", j = ");
    printInt(j);
    printNewLine();
}

void b(void)
{
    printString("in b()\n");
    a();
}

int fibonacci(int n)
{
    if (n == 1)
        return 1;
    else if (n == 2)
        return 2;
    else
        return fibonacci(n-1) + fibonacci(n-2);
}

void main(void)
{
    a();
    b();
    for (k = 1; k <= 10; k++) {
        printString("Fibonnaci number ");
        printInt(k);
        printString(" is ");
        printInt(fibonacci(k));
        printNewLine();
    }
}
