echo 0x03030300 > /sys/kernel/debug/lophilo/power
echo 0xFFFFFFFF > /sys/kernel/debug/lophilo/gpio1/doe
PWMDIR=/sys/kernel/debug/lophilo

LIST=`find $PWMDIR -name "pwm*"`

for PWM in $LIST
do
	echo 0x0 > $PWM/reset
	echo 0x0000FFFF > $PWM/gate
	echo $((RANDOM%65535)) > $PWM/dtyc
done
