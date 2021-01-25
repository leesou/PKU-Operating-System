#include "syscall.h"

#define len 16 // hello,my world!\a

int main()
{
    int fd_read, fd_write, fd2;

    char name[9];
    char name1[6];

    char buffer[13];
    char out_buffer[13];

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

    name1[0] = 'h';
    name1[1] = 'e';
    name1[2] = 'l';
    name1[3] = 'l';
    name1[4] = 'o';
    name1[5] = '\0';
    fd2 = Open(name1);


    Read(buffer, len, fd2);
    Write(buffer, len, fd_write);

    fd_read = Open(name);
    Read(out_buffer, len, fd_read);
    Write(out_buffer, len, fd2); // write after content

    Close(fd_read);
    Close(fd_write);
    Close(fd2);
}