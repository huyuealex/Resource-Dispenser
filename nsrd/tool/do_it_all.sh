echo cfq > /sys/block/sdb/queue/scheduler
cat /sys/block/sdb/queue/scheduler
rmmod rd-iosched
cd nsrd/
./build.sh
insmod rd-iosched.ko
cd ../
gcc -o new_iotest new_iotest.c
cp -arf new_iotest ../battle/
cd ../battle/
echo nsrd > /sys/block/sdb/queue/scheduler
cat /sys/block/sdb/queue/scheduler
echo t 10007 > /proc/rdnice
cat /proc/rdnice
./new_iotest
