#!/bin/sh
source /root/wbc-scripts/env.sh
source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

if [[ "$OPENWRT_RSSI_FORWARD_PORT" == "null" ]]; then
	echo "RSSI RX: Disabled."
	sleep 365d
fi

tmessage "RSSI Forwarder listen at localhost:$OPENWRT_RSSI_FORWARD_PORT"
/root/wifibroadcast/rssi_forward_in $OPENWRT_RSSI_FORWARD_PORT

echo "Stopped."
