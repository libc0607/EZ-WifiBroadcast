#!/bin/bash

source /root/wbc-scripts/env.sh

# Apply OpenWrt config
if [ -e "$OPENWRT_CONFIG_FILE" ]; then
	
    OK=`bash -n $OPENWRT_CONFIG_FILE`
    if [ "$?" == "0" ]; then
		tmessage "OpenWrt config file $OPENWRT_CONFIG_FILE found"
		source $OPENWRT_CONFIG_FILE
    else
		tmessage "ERROR: OpenWrt config file $OPENWRT_CONFIG_FILE contains syntax error(s)"
		exit
    fi
else
    tmessage "ERROR: OpenWrt config file not found "
    exit
fi

if [ "$OPENWRT_VIDEO_FPS" == "59.9" ]; then
    DISPLAY_PROGRAM=/opt/vc/src/hello_pi/hello_video/hello_video.bin.48-mm
else
    if [ "$OPENWRT_VIDEO_FPS" -eq 30 ]; then
	DISPLAY_PROGRAM=/opt/vc/src/hello_pi/hello_video/hello_video.bin.30-mm
    fi
    if [ "$OPENWRT_VIDEO_FPS" -lt 60 ]; then
	DISPLAY_PROGRAM=/opt/vc/src/hello_pi/hello_video/hello_video.bin.48-mm
    fi
    if [ "$OPENWRT_VIDEO_FPS" -gt 60 ]; then
	DISPLAY_PROGRAM=/opt/vc/src/hello_pi/hello_video/hello_video.bin.240-befi
    fi
fi
