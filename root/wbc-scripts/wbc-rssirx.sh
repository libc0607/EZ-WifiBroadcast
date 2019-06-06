#!/bin/sh
source /root/wbc-scripts/env.sh
# /root/.profile - main EZ-Wifibroadcast script
# (c) 2017 by Rodizio. Licensed under GPL2
#
# Mod for using OpenWrt, by libc0607@Github
#

source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

if [[ "$OPENWRT_RSSI_FORWARD_PORT" == "null" ]]; then
	tmessage "RSSI RX: Disabled."
	sleep 365d
fi

tmessage "RSSI Forwarder listen at localhost:$OPENWRT_RSSI_FORWARD_PORT"
/root/wifibroadcast/rssi_forward_in $OPENWRT_RSSI_FORWARD_PORT

tmessage "Stopped."
