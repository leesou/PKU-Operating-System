#!/bin/sh

cd ../

echo "=== reset file system ==="
./nachos -V -f

echo ""
echo "=== move a normal file to file system ==="
./nachos -V -cp test/hello hello

echo ""
echo "=== move executable file to file system ==="
./nachos -V -cp ../test/FileSyscall FileSyscall

echo ""
echo "=== show file system contents"
./nachos -V -D

echo ""
echo "=== test file system ==="
./nachos -V -x FileSyscall -d S
# delete vm, later will implement in exit
./nachos -V -rm VirtualMemory0 

echo ""
echo "=== show file system contents"
./nachos -V -D

