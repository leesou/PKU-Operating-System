#!bin/sh

echo "=== Generate middle-sized file for single indirect indexing ==="
dd if=/dev/urandom of=midFile count=3 bs=1024 # 3KB

# this file saves in filesys/test/
cd ../

echo "=== initialize Nachos file system ==="
./nachos -V -f

echo "=== copies file \"big\" from filesys/test/ to Nachos DISK (and add extension) ==="
./nachos -V -cp test/midFile midFile.efg

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D

sleep 2

echo ""
echo "=== remove previous file ==="
./nachos -V -r midFile.efg

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D
