#include "syscall.h"

int
main()
{
	int n;
	int haha = 0;
	for (n=0;n<1000000;n++) {
		haha++;
	}
    Exit(1);
}

