#!/bin/sh

# this file saves in filesys/test/
cd ../

echo "=== copies file \"small\" from filesys/test/ to Nachos DISK (and add extension) ==="
./nachos -V -cp test/small small.abc

sleep 2 # to observe the modification time change

echo ""
echo "=== Read content of the file in Nachos DISK ==="
./nachos -V -p small.abc

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D

sleep 2

echo ""
echo "=== remove previous file ==="
./nachos -V -r small.abc

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D
