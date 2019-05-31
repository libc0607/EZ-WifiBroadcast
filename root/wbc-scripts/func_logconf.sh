#!/bin/bash

source /root/wbc-scripts/env.sh

if [ -e "$OPENWRT_LOGIN_FILE" ]; then
	tmessage "OpenWrt login config file $OPENWRT_LOGIN_FILE "
	dos2unix -n $OPENWRT_LOGIN_FILE /tmp/settings.sh
    OK=`bash -n /tmp/settings.sh`
    if [ "$?" == "0" ]; then
		source /tmp/settings.sh
		tmessage "IP: $OPENWRT_IP, Username: $OPENWRT_USERNAME, Password: $OPENWRT_PASSWORD"
    else
		tmessage "ERROR: config file contains syntax error(s)!"
		exit
    fi
else
    tmessage "ERROR: config file not found!"
    exit
fi
