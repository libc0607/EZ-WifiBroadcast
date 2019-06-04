#!/bin/bash
# Note: This script should be called when startup.
# You should add 'bash /path/to/this/script.sh' to /etc/rc.local.
CAM=`/usr/bin/vcgencmd get_camera | nice grep -c detected=1`
rm /etc/supervisor/conf.d/wbc-air.conf
rm /etc/supervisor/conf.d/wbc-ground.conf
if [ "$CAM" == "0" ]; then
	echo "0" > /tmp/cam
	echo "Camera not found."
	ln -s /root/wbc-scripts/wbc-ground.conf /etc/supervisor/conf.d/wbc-ground.conf
	/root/wifibroadcast/sharedmem_init_rx
else
	touch /tmp/TX
	echo "1" > /tmp/cam
	echo "Camera found."
	ln -s /root/wbc-scripts/wbc-air.conf /etc/supervisor/conf.d/wbc-air.conf
fi

supervisorctl update all
/etc/init.d/supervisor restart

#end
