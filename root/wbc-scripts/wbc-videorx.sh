#!/bin/sh
source /root/wbc-scripts/env.sh
source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

if vcgencmd get_throttled | nice grep -q -v "0x0"; then
	TEMP=`cat /sys/class/thermal/thermal_zone0/temp`
	TEMP_C=$(($TEMP/1000))
	if [ "$TEMP_C" -lt 75 ]; then
		echo "ERROR: Under-Voltage detected on the RX Pi. Your Pi is not supplied with stable 5 Volts."
		echo "Either your power-supply or wiring is not sufficent, check the wiring instructions in the Wiki!"
		echo "1" > /tmp/undervolt
		sleep 1
	fi
else
	echo "0" > /tmp/undervolt
fi

if [[ "$OPENWRT_VIDEO_FORWARD_PORT" == "null" ]]; then
	echo "Video RX: Disabled."
	sleep 365d
fi
echo "Starting display ... "

# socat -u UDP-LISTEN:$OPENWRT_VIDEO_FORWARD_PORT EXEC:'$DISPLAY_PROGRAM' 
/opt/vc/src/hello_pi/hello_video/hello_video.bin.120-udp $OPENWRT_VIDEO_FORWARD_PORT 
