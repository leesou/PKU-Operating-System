#include "syscall.h"

char str[5];
int fd;

void func()
{
    int fd1;
    char name[10];
    name[0] = 't'; name[1] = 'e'; name[2] = 's', name[3] = 't'; name[4] = '2';
    name[5] = '.'; name[6] = 't'; name[7] = 'x'; name[8] = 't'; name[9] = '\0';
    Write(str, 4, fd);

    Create(name);
    fd1 = Open(name);
    Write(str, 4, fd1);
    Exit(fd);
}

void main()
{
    char name[10];
    name[0] = 't'; name[1] = 'e'; name[2] = 's', name[3] = 't'; name[4] = '1';
    name[5] = '.'; name[6] = 't'; name[7] = 'x'; name[8] = 't'; name[9] = '\0';

    str[0] = 't'; str[1] = 'e'; str[2] = 's'; str[3] = 't'; str[4] = '\0';
    Create(name);
    fd = Open(name);
    Write(str, 4, fd);
    str[0] = 'T';
    Fork(func);
    str[1] = 'E';
    Exit(fd);
}