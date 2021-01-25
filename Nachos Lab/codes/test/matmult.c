/* matmult.c 
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define Dim 	16	

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int
main()
{
    int i, j, k;
	int cnt=0;
	char test[6];
	test[0] = 't'; test[1] = 'e'; test[2] = 's'; test[3] = 't'; 
	test[4] = '\n'; test[5] = '\0';

    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
		for (j = 0; j < Dim; j++) 
		{
	     	A[i][j] = i;
	     	B[i][j] = j;
	     	C[i][j] = 0;

			cnt++;
			if(cnt % 128 == 0)
			{
				Yield();
				Write(test, 6, ConsoleOutput);
			}
		}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
		for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
			{
				C[i][j] += A[i][k] * B[k][j];

				cnt++;
				if(cnt % 128 == 0)
				{
					Yield();
					Write(test, 6, ConsoleOutput);
				}
			}
		 		

    Exit(C[Dim-1][Dim-1]);		/* and then we're done */
}
