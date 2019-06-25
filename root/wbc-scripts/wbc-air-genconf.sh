#!/bin/bash
OPENWRT_VIDEO_FORWARD_PORT=35000	
OPENWRT_VIDEO_FPS=48
OPENWRT_VIDEO_KEYFRAMERATE=5
OPENWRT_VIDEO_WIDTH=800
OPENWRT_VIDEO_HEIGHT=480
OPENWRT_VIDEO_BITRATE=1024000

CONF=$(cat /var/run/wbc/config.ini)
OPENWRT_VIDEO_FORWARD_PORT=`echo $CONF |grep video_forward_port|cut -d '=' -f 2`
OPENWRT_VIDEO_FPS=`echo $CONF |grep video_fps|cut -d '"' -f 2`
OPENWRT_VIDEO_KEYFRAMERATE=`echo $CONF |grep video_keyframerate|cut -d '"' -f 2`
OPENWRT_VIDEO_WIDTH=`echo $CONF |grep video_size |cut -d '"' -f 2|cut -d 'x' -f 1`
OPENWRT_VIDEO_HEIGHT=`echo $CONF |grep video_size|cut -d '"' -f 2|cut -d 'x' -f 2`
OPENWRT_VIDEO_BITRATE=`echo $CONF |grep video_bitrate|cut -d '"' -f 2`

# save them to file
echo "#!/bin/bash" > $OPENWRT_CONFIG_FILE
echo "# Auto generated at $(date)" # make sure the hash of this file change
echo "OPENWRT_VIDEO_FORWARD_PORT=$OPENWRT_VIDEO_FORWARD_PORT" >> $OPENWRT_CONFIG_FILE
echo "OPENWRT_VIDEO_FPS=$OPENWRT_VIDEO_FPS" >> $OPENWRT_CONFIG_FILE
echo "OPENWRT_VIDEO_KEYFRAMERATE=$OPENWRT_VIDEO_KEYFRAMERATE" >> $OPENWRT_CONFIG_FILE
echo "OPENWRT_VIDEO_WIDTH=$OPENWRT_VIDEO_WIDTH" >> $OPENWRT_CONFIG_FILE
echo "OPENWRT_VIDEO_HEIGHT=$OPENWRT_VIDEO_HEIGHT" >> $OPENWRT_CONFIG_FILE
echo "OPENWRT_VIDEO_BITRATE=$OPENWRT_VIDEO_BITRATE" >> $OPENWRT_CONFIG_FILE
