#!/bin.sh

cd ../

echo "=== clean file system ==="
./nachos -V -f

echo ""
echo "=== move executable file to file system ==="
./nachos -V -cp ../test/testThread testThread
./nachos -V -cp ../test/matmult matmult

echo ""
echo "=== show file system contents ==="
./nachos -V -D

echo ""
echo "=== try to run test program ==="
./nachos -V -x testThread -d S

echo ""
echo "=== show file system contents"
./nachos -V -D
