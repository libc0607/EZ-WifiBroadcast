#!/bin/bash
source /root/wbc-scripts/env.sh
source /root/wbc-scripts/func_logconf.sh
# pid 
echo $$ > $1

while true; do
	wget http://$OPENWRT_IP/$OPENWRT_CONF_PATH -O /var/run/wbc/config.ini
	sleep 3
done


