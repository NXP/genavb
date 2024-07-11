#!/bin/bash

source /etc/genavb/salsacam-configs.inc

CAMCTRL=salsacamctrl

if [ $# -ne 1 ]; then
	echo "Usage: $0 <camera number>"
	exit 1
fi
CAM=$1

if [ "${IP[$CAM]}" = "" ]; then
	echo "Unknown camera number $CAM"
	exit 1
fi


echo "Configuring camera $CAM ..."
$CAMCTRL -i $DEFAULT_IP -I ${IP[$CAM]} -M ${MAC[$CAM]} -A ${SID[$CAM]} -B $RATE -Z 1 -T 1

echo
echo "Rebooting camera $CAM ..."
$CAMCTRL -i $DEFAULT_IP -R
echo "Waiting for camera to reboot..."
sleep 4

echo
echo "Checking camera $CAM configuration:"
$CAMCTRL -i ${IP[$CAM]} -m -a -b -z -t -y
