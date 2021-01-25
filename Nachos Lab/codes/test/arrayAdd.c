#include "syscall.h"

#define T 2
#define N 40

int
main()
{
    int t, i;
    int A[N], B[N];
    for(t=0; t<T; ++t)
    {
        A[0] = 1; B[0] = 1;
        A[1] = 2; B[1] = 2;
        for(i=2; i<N; ++i)
        {
            A[i] = A[i-1] + A[i-2];
            B[i] = B[i-1] + B[i-2];
        }
    }
    Halt();
}
