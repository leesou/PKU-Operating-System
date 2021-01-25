#include "syscall.h"

#define len 17 //test_for_console.

int main()
{
    int fd_read, fd_write, num;

    char in_buffer[len];
    char out_buffer[len];
    char name[9];
    name[0] = 't';
    name[1] = 'e';
    name[2] = 's';
    name[3] = 't';
    name[4] = '.';
    name[5] = 't';
    name[6] = 'x';
    name[7] = 't';
    name[8] = '\0';

    Create(name);
    fd_write = Open(name);

    num = Read(in_buffer, len, ConsoleInput);
    Write(in_buffer, num, fd_write);

    fd_read = Open(name);
    num = Read(out_buffer, len, fd_read);
    Write(out_buffer, num, ConsoleOutput);

    Close(fd_read);
    Close(fd_write);
}

