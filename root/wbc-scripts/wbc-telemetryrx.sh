#!/bin/sh
source /root/wbc-scripts/env.sh
source /root/wbc-scripts/func_logconf.sh
source /root/wbc-scripts/func_applyconf.sh

tmessage "Telemetry RX: listen on $OPENWRT_TELE_FORWARD_PORT"
socat -u UDP-LISTEN:$OPENWRT_TELE_FORWARD_PORT OPEN:/root/telemetryfifo1
tmessage "Stopped."

