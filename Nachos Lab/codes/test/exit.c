#include "syscall.h"

int
main()
{   
    int i=514, A[1];
    for(i=0; i<514; ++i)
    {
        A[0] = i;
    }
    Exit(A[0]);
    /* not reached */
}
