echo 0x03030300 > /sys/kernel/debug/lophilo/power
echo 0xFFFFFFFF > /sys/kernel/debug/lophilo/a/gpio/doe
LIST=`ls -v /sys/kernel/debug/lophilo/a/gpio/io*`
for PIN in $LIST
do
	echo 0x0 > $PIN
	sleep 0.01
	echo 0xFF > $PIN
done
echo 0x0 > /sys/kernel/debug/lophilo/a/gpio/doe
echo 0x0 > /sys/kernel/debug/lophilo/power
