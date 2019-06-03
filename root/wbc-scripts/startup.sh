#!/bin/bash

CAM=`/usr/bin/vcgencmd get_camera | nice grep -c detected=1`
if [ "$CAM" == "0" ]; then 
	echo "0" > /tmp/cam
	echo "Camera not found."
	ln -s /root/wbc-scripts/wbc-ground.conf /etc/supervisor/conf.d/wbc-ground.conf
else 
	touch /tmp/TX
	echo "1" > /tmp/cam
	echo "Camera found."
	ln -s /root/wbc-scripts/wbc-air.conf /etc/supervisor/conf.d/wbc-air.conf
fi 
/etc/init.d/supervisor restart

