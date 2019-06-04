#!/bin/bash
source /root/wbc-scripts/env.sh

# check default config
tmessage "======================================"
tmessage "wbc-videotx-raspivid $(date)"
tmessage "======================================"

source /root/wbc-scripts/func_logconf.sh

# Apply OpenWrt config
source /root/wbc-scripts/func_applyconf.sh

tmessage "Config done, starting ... "


if vcgencmd get_throttled | nice grep -q -v "0x0"; then
	TEMP=`cat /sys/class/thermal/thermal_zone0/temp`
	TEMP_C=$(($TEMP/1000))
	if [ "$TEMP_C" -lt 75 ]; then # it must be under-voltage
		tmessage
		tmessage "  ---------------------------------------------------------------------------------------------------"
		tmessage "  | ERROR: Under-Voltage detected on the TX Pi. Your Pi is not supplied with stable 5 Volts.        |"
		tmessage "  | Either your power-supply or wiring is not sufficent, check the wiring instructions in the Wiki. |"
		tmessage "  | Set Video Bitrate lower in LuCI (~1000kbit) to reduce current consumption!                      |"
		tmessage "  ---------------------------------------------------------------------------------------------------"
		tmessage
		mount -o remount,rw /boot
		echo "  ---------------------------------------------------------------------------------------------------" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		echo "  | ERROR: Under-Voltage detected on the TX Pi. Your Pi is not supplied with stable 5 Volts.        |" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		echo "  | Either your power-supply or wiring is not sufficent, check the wiring instructions in the Wiki. |" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		echo "  | Set Video Bitrate lower in LuCI (~1000kbit) to reduce current consumption!                      |" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		echo "  | When you have fixed wiring/power-supply, delete this file and make sure it doesn't re-appear!   |" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		echo "  ---------------------------------------------------------------------------------------------------" >> /boot/UNDERVOLTAGE-ERROR!!!.txt
		mount -o remount,ro /boot
		UNDERVOLT=1
		echo "1" > /tmp/undervolt
	else # it was either over-temp or both undervolt and over-temp, we set undervolt to 0 anyway, since overtemp can be seen at the temp display on the rx
		UNDERVOLT=0
		echo "0" > /tmp/undervolt
	fi
else
	UNDERVOLT=0
	echo "0" > /tmp/undervolt
	tmessage "Power good."
fi

#echo $OPENWRT_VIDEO_BITRATE > /tmp/bitrate_kbit
tmessage "Starting video transmission, $OPENWRT_VIDEO_WIDTHx$OPENWRT_VIDEO_HEIGHT $OPENWRT_VIDEO_FPS fps, video bitrate: $OPENWRT_VIDEO_BITRATE Bit/s, Keyframerate: $OPENWRT_VIDEO_KEYFRAMERATE"
tmessage "Video stream send to $OPENWRT_IP:$OPENWRT_VIDEO_FORWARD_PORT"

# "raspivid -o udp://" seems can't bind to a specified port
# a random port after raspivid restart could cause video "connection" lose 
#nice -n -9 raspivid -w $OPENWRT_VIDEO_WIDTH -h $OPENWRT_VIDEO_HEIGHT -fps $OPENWRT_VIDEO_FPS -b $OPENWRT_VIDEO_BITRATE -g $OPENWRT_VIDEO_KEYFRAMERATE -t 0 $EXTRAPARAMS -o udp://$OPENWRT_IP:$OPENWRT_VIDEO_FORWARD_PORT

#RASPIVIDCMD="raspivid -w $OPENWRT_VIDEO_WIDTH -h $OPENWRT_VIDEO_HEIGHT -fps $OPENWRT_VIDEO_FPS -b $OPENWRT_VIDEO_BITRATE -g $OPENWRT_VIDEO_KEYFRAMERATE -t 0 $EXTRAPARAMS -o -"
#socat -u EXEC:"$RASPIVIDCMD" UDP:$OPENWRT_IP:$OPENWRT_VIDEO_FORWARD_PORT,sourceport=30000

socat -u EXEC:"raspivid -w $OPENWRT_VIDEO_WIDTH -h $OPENWRT_VIDEO_HEIGHT -fps $OPENWRT_VIDEO_FPS -b $OPENWRT_VIDEO_BITRATE -g $OPENWRT_VIDEO_KEYFRAMERATE -t 0 -cd H264 -n -fl -ih -pf high -if both -ex sports -mm average -awb horizon -o -" UDP:$OPENWRT_IP:$OPENWRT_VIDEO_FORWARD_PORT,sourceport=30000

