#include "syscall.h"

int main()
{
    char name[8];
    SpaceId TID;
    int status;

    name[0] = 'm'; name[1] = 'a'; name[2] = 't'; name[3] = 'm';
    name[4] = 'u'; name[5] = 'l'; name[6] = 't'; name[7] = '\0';

    TID = Exec(name);
    status = Join(TID);
    Exit(status);
}