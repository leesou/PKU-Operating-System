#!/bin/sh

# this file saves in filesys/test/
cd ../

# use -V to disable verbose machine messages
echo "=== format the DISK ==="
./nachos -V -f

echo ""
echo ""
echo "=== create a directory called \"folder\" ==="
./nachos -V -d D -mkdir folder

echo ""
echo ""
echo "=== create a directory called \"tryr\" ==="
./nachos -V -d D -mkdir try

echo ""
echo ""
echo "=== create additional two directories called \"test1\" \"test2\" in \"folder\" ==="
./nachos -V -d D -mkdir folder/test1
./nachos -V -d D -mkdir folder/test2

echo ""
echo ""
echo "=== create additional directoriy called \"dir\" in \"folder/test1\" ==="
./nachos -V -d D -mkdir folder/test1/dir

echo ""
echo ""
echo "=== copy file \"big\" to \"folder\" ==="
./nachos -V -d D -cp test/big folder/big
./nachos -V -d D -p folder/big

echo ""
echo ""
echo "=== copy file \"medium\" to \"folder/test1/dir/medium\" ==="
./nachos -V -d D -cp test/medium folder/test1/dir/medium
./nachos -V -d D -p folder/test1/dir/medium

echo ""
echo ""
echo "=== copy file \"small\" to \"folder/test2/small\" ==="
./nachos -V -d D -cp test/small folder/test2/small
./nachos -V -d D -p folder/test2/small

echo ""
echo ""
echo "=== copy file \"small\" to \"small\" ==="
./nachos -V -d D -cp test/small small
./nachos -V -d D -p small

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
echo "=== list folder/try ==="
./nachos -V -ls folder/try

echo ""
echo ""
echo "=== list folder/test1/dir ==="
./nachos -V -ls folder/test1/dir