#!/bin/sh

cd ../

echo "=== clean file system ==="
./nachos -V -f

echo ""
echo "=== move executable file to file system ==="
./nachos -V -cp ../test/testConsole testConsole

echo ""
echo "=== show file system contents"
./nachos -V -D

echo ""
echo "=== try to create a file ==="
./nachos -V -x testConsole -d S
# delete vm, later will implement in exit
./nachos -V -rm VirtualMemory0

echo ""
echo "=== show file system contents"
./nachos -V -D

