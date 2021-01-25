#!bin/sh

# this file saves in filesys/test/
cd ../

echo "=== initialize Nachos file system ==="
./nachos -V -f

echo "=== copies file \"big\" from filesys/test/ to Nachos DISK (and add extension) ==="
./nachos -V -cp test/big big.abc

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D

sleep 2

echo ""
echo "=== remove previous file ==="
./nachos -V -r big.abc

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D
