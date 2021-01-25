#!/bin/sh

cd ../
# this script must run after test4.sh

echo ""
echo ""
echo "=== try to directly remove a unempty folder in root dir, which should fail ==="
./nachos -V -d D -r folder

echo ""
echo ""
echo "=== try to directly remove an empty folder in root dir, named with try ==="
./nachos -V -d D -r try

echo ""
echo ""
echo "=== try to remove small in root ==="
./nachos -V -d D -r small

echo ""
echo ""
echo "=== try to remove the file \"folder/test2/small\" ==="
./nachos -V -d D -rm folder/test2/small

echo ""
echo ""
echo "=== try to remove the directory \"folder/test2\" ==="
./nachos -V -d D -r folder/test2


echo ""
echo ""
echo "=== try to remove the directory \"folder\" ==="
./nachos -V -d D -rm folder

echo ""
echo ""
echo "=== try to remove the directory \"folder/test1/dir\" ==="
echo "=== remove the file in \"folder/test1/dir\" first ==="
./nachos -V -d D -rm folder/test1/dir/medium
echo ""
./nachos -V -d D -rm folder/test1/dir

echo ""
echo ""
echo "=== list all files and directories in root directory ==="
./nachos -V -l

echo ""
echo ""
echo "=== list folder ==="
./nachos -V -ls folder

echo ""
echo ""
echo "=== list folder/test1 ==="
./nachos -V -ls folder/test1

echo ""
echo ""
echo "=== list folder/test2 ==="
./nachos -V -ls folder/test2

echo ""
echo ""
echo "=== list folder/test1/dir ==="
./nachos -V -ls folder/test1/dir