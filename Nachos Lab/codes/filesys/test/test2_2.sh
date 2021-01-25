#!/bin/sh

# this file saves in filesys/test/
cd ../

echo "=== reset the file system  ==="
./nachos -f -D

echo "=== copy to Nachos file with long filename  ==="
./nachos -V -cp test/small this_is_a_very_very_very_very_very_very_very_very_very_very_very_very_long_name.txt

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D
