echo 0x03030300 > /sys/kernel/debug/lophilo/power
echo 0xFFFFFFFF > /sys/kernel/debug/lophilo/b/gpio/doe
PWMDIR=/sys/kernel/debug/lophilo/b/pwm
LIST=`ls -v $PWMDIR`
for PIN in $LIST
do
	echo 0x0 > $PWMDIR/$PIN/reset
	echo 0x0000FFFF > $PWMDIR/$PIN/gate
	echo $((RANDOM%65535)) > $PWMDIR/$PIN/dtyc
done
