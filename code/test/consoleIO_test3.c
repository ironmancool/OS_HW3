#include "syscall.h"

int
main()
{
	int n;
	/*PrintInt(1 << 31);
    PrintInt(2147483647);*/
    for (n = 0; n < 10000; n++)
        if (n == 100) PrintInt(n);
    Exit(3);
}
