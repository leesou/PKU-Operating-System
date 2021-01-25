#!/bin.sh

cd ../

echo "=== clean file system ==="
./nachos -V -f

echo ""
echo "=== move executable file to file system ==="
./nachos -V -cp ../test/testFork testFork

echo ""
echo "=== show file system contents ==="
./nachos -V -D

echo ""
echo "=== try to run test program ==="
./nachos -V -x testFork -d S

echo ""
echo "=== show file system contents"
./nachos -V -D
