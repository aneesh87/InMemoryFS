# Build
make clean
make

# CleanUP
fusermount -u /tmp/fuse
rm -rf /tmp/fuse
mkdir /tmp/fuse
#rm /home/agupta27/log.txt

# Run
./ramdisk /tmp/fuse 512 /home/agupta27/save1.bin

#./ramdisk /tmp/fuse 512
# Test
#echo "\nls -l /tmp/fuse"
#ls -l /tmp/fuse

#Test
echo "\ncat /tmp/fuse/hello"
cat /tmp/fuse/hello

echo "\ncat /home/agupta27/log.txt"
cat /home/agupta27/log.txt
