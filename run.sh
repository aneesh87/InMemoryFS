# Build
make clean
make

# CleanUP
fusermount -u /tmp/fuse
rm -rf /tmp/fuse
mkdir /tmp/fuse

# Run
./ramdisk /tmp/fuse 512 /home/agupta27/save1.bin

#./ramdisk /tmp/fuse 512

# Test
echo "\nls -ltR /tmp/fuse"
ls -ltR /tmp/fuse


