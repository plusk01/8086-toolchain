/* Last updated 16 September 2003 */

#include "clib.h"

#define STARTVAL 1001U
#define ENDVAL 65000U

void main(void)
{
    unsigned i, j;
    int flag,lncnt;

    printString("Primes between ");
    printUInt(STARTVAL);
    printString(" and ");
    printUInt(ENDVAL);
    printString(":\n");
    lncnt = 0;

    for (i = STARTVAL; i < ENDVAL; i += 2) {
        flag = 0;
        for (j = 3; (j*j) < i; j += 2) {
            if (i % j == 0) {
                flag = 1;
                break;
            }
        }
        if (!flag) {
            printChar(' ');
            printUInt(i);
            lncnt++;
            if (lncnt > 9) {
                printNewLine();
                lncnt = 0;
            }
        }
    }

    printNewLine();
}
