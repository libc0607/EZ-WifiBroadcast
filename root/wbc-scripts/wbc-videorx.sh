#!/bin/sh
source /root/wbc-scripts/env.sh
# /root/.profile - main EZ-Wifibroadcast script
# (c) 2017 by Rodizio. Licensed under GPL2
#
# Mod for using OpenWrt, by libc0607@Github
#

source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

if vcgencmd get_throttled | nice grep -q -v "0x0"; then
	TEMP=`cat /sys/class/thermal/thermal_zone0/temp`
	TEMP_C=$(($TEMP/1000))
	if [ "$TEMP_C" -lt 75 ]; then
		tmessage "  ---------------------------------------------------------------------------------------------------"
		tmessage "  | ERROR: Under-Voltage detected on the RX Pi. Your Pi is not supplied with stable 5 Volts.        |"
		tmessage "  |                                                                                                 |"
		tmessage "  | Either your power-supply or wiring is not sufficent, check the wiring instructions in the Wiki! |"
		tmessage "  ---------------------------------------------------------------------------------------------------"
		echo "1" > /tmp/undervolt
		sleep 1
	fi
else
	echo "0" > /tmp/undervolt
fi

tmessage "Starting RX ... "

socat -u UDP-LISTEN:$OPENWRT_VIDEO_FORWARD_PORT EXEC:'$DISPLAY_PROGRAM' 

