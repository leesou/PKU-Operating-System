# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as

GCCDIR = ../../../gnu-decstation-ultrix/decstation-ultrix/2.95.3/

LDFLAGS = -T script -N
ASFLAGS = -mips2
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

PATH = $(GCCDIR):/lib:/usr/bin:/bin

CC = $(GCCDIR)gcc -B../../../gnu-decstation-ultrix/
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

CPP = gcc -E
INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt shell matmult sort arrayAdd exit Create FileSyscall testConsole testThread testFork

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.c > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

arrayAdd.o: arrayAdd.c
	$(CC) $(CFLAGS) -c arrayAdd.c
arrayAdd: arrayAdd.o start.o
	$(LD) $(LDFLAGS) start.o arrayAdd.o -o arrayAdd.coff
	../bin/coff2noff arrayAdd.coff arrayAdd

exit.o: exit.c
	$(CC) $(CFLAGS) -c exit.c
exit: exit.o start.o
	$(LD) $(LDFLAGS) start.o exit.o -o exit.coff
	../bin/coff2noff exit.coff exit

Create.o: Create.c
	$(CC) $(CFLAGS) -c Create.c
Create: Create.o Create.o
	$(LD) $(LDFLAGS) start.o Create.o -o Create.coff
	../bin/coff2noff Create.coff Create

FileSyscall.o: FileSyscall.c
	$(CC) $(CFLAGS) -c FileSyscall.c
FileSyscall: FileSyscall.o FileSyscall.o
	$(LD) $(LDFLAGS) start.o FileSyscall.o -o FileSyscall.coff
	../bin/coff2noff FileSyscall.coff FileSyscall

testConsole.o: testConsole.c
	$(CC) $(CFLAGS) -c testConsole.c
testConsole: testConsole.o testConsole.o
	$(LD) $(LDFLAGS) start.o testConsole.o -o testConsole.coff
	../bin/coff2noff testConsole.coff testConsole

testThread.o: testThread.c
	$(CC) $(CFLAGS) -c testThread.c
testThread: testThread.o testThread.o
	$(LD) $(LDFLAGS) start.o testThread.o -o testThread.coff
	../bin/coff2noff testThread.coff testThread

testFork.o: testFork.c
	$(CC) $(CFLAGS) -c testFork.c
testFork: testFork.o testFork.o
	$(LD) $(LDFLAGS) start.o testFork.o -o testFork.coff
	../bin/coff2noff testFork.coff testFork