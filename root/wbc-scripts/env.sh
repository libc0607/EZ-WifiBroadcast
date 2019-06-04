#!/bin/bash

QUIET=N
# save config to these file
OPENWRT_LOGIN_FILE=/boot/opconfig.txt
OPENWRT_CONFIG_FILE=/tmp/wbc/openwrt_config.sh
OPENWRT_TOKEN_FILE=/tmp/wbc/openwrt_auth_token
OPENWRT_RESTART_TS_FILE=/tmp/wbc/openwrt_restart_timestamp
OPENWRT_CONFIG_MD5_FILE=/tmp/wbc/openwrt_config_md5
OPENWRT_CONFIGURED_FLAG_FILE=/tmp/wbc_openwrt_configured
CHECK_ALIVE_FILE=/tmp/wbc_openwrt_alive
sudo mkdir -p /tmp/wbc/
#[ -f $OPENWRT_RESTART_TS_FILE ] && sudo echo "" > $OPENWRT_RESTART_TS_FILE || echo -n ""
#[ -f $OPENWRT_TOKEN_FILE ] && sudo echo "" > $OPENWRT_TOKEN_FILE || echo -n "" 
#[ -f $OPENWRT_CONFIG_MD5_FILE ] && sudo echo "" > $OPENWRT_CONFIG_MD5_FILE || echo -n ""
#[ -f $OPENWRT_CONFIGURED_FLAG_FILE ] && sudo echo "" > $OPENWRT_CONFIGURED_FLAG_FILE || echo -n ""
#[ -f $CHECK_ALIVE_FILE ] && sudo echo "" > $CHECK_ALIVE_FILE || echo -n ""

function tmessage {
    if [ "$QUIET" == "N" ]; then
		echo $1 "$2"
    fi
}