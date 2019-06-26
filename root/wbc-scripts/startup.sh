#!/bin/bash
# Note: This script should be called when startup.
# You should add 'bash /path/to/this/script.sh' to /etc/rc.local.

CAM=`/usr/bin/vcgencmd get_camera | nice grep -c detected=1`
rm /etc/monit/conf.d/wbc-air.conf 2>/dev/null
rm /etc/monit/conf.d/wbc-ground.conf 2>/dev/null
echo "0" >/tmp/undervolt
if [ "$CAM" == "0" ]; then
	echo "0" > /tmp/cam
	echo "Camera not found."
	ln -s /root/wbc-scripts/wbc-ground-monit.conf /etc/monit/conf.d/wbc-ground.conf
	/root/wifibroadcast/sharedmem_init_rx
else
	touch /tmp/TX
	echo "1" > /tmp/cam
	echo "Camera found."
	ln -s /root/wbc-scripts/wbc-air-monit.conf /etc/monit/conf.d/wbc-air.conf
fi
dos2unix -n /boot/opconfig.txt /tmp/settings.sh
monit reload
monit restart all
echo Done.
#end
