#!bin/sh

echo "=== Generate large file to test double-level indirect directory"
dd if=/dev/zero of=largeFile count=20 bs=1024 # 20KB

cd ../
make

echo "=== initialize Nachos system ==="
./nachos -f -V

echo "=== copy largeFile to Nachos's file system ==="
./nachos -V -cp test/largeFile largeFile

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D

sleep 2

echo ""
echo "=== remove previous file ==="
./nachos -V -r largeFile

echo ""
echo "=== prints the contents of the entire file system ==="
./nachos -V -D
