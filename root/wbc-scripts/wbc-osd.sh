#!/bin/sh
source /root/wbc-scripts/env.sh
source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

OSD_CONFIG_FILE_MD5_PATH=/var/run/wbc/osdconfig_md5
OSD_CONFIG_FILE_MD5_NEW=`md5sum /boot/osdconfig.txt|cut -d ' ' -f 1`
OSD_CONFIG_FILE_MD5_OLD=`cat $OSD_CONFIG_FILE_MD5_PATH`

if [[ "$OSD_CONFIG_FILE_MD5_OLD" != "$OSD_CONFIG_FILE_MD5_NEW" ]]; then
	tmessage "Config changed, rebuilding.. "
	ionice -c 3 nice dos2unix -n /boot/osdconfig.txt /tmp/osdconfig.txt
	cd /root/wifibroadcast-osd
	ionice -c 3 nice make || {
		tmessage "ERROR: Could not build OSD, check osdconfig.txt!"
		sleep 1
		nice /root/wifibroadcast_status/wbc_status "ERROR: Could not build OSD, check osdconfig.txt for errors." 7 55 0
		sleep 1
	}
	echo $OSD_CONFIG_FILE_MD5_NEW > $OSD_CONFIG_FILE_MD5_PATH
else
	tmessage "Config not changed, skip rebuilding.. "
fi

killall wbc_status > /dev/null 2>&1
tmessage "Waiting until OpenWrt rx is running ..."
VIDEORXRUNNING=0
while [[ "$VIDEORXRUNNING" != "1" ]]; do
	sleep 1
	VIDEORXRUNNING=`cat $CHECK_ALIVE_FILE`
done
tmessage "Video running, starting OSD processes ..."

/tmp/osd $OPENWRT_TELE_FORWARD_PORT > /dev/null

tmessage "OSD Stopped."
