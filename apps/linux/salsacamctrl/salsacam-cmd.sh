#! /bin/bash

source /etc/genavb/salsacam-configs.inc

usage () {
	echo "Usage:"
	echo "		`basename $0` <command> <N> to send <command> to camera N"
	echo "		`basename $0` <command> to send <command> to all cameras"
	echo ""
	echo "Allowed commands:"
	echo "		start	: start streaming"
	echo "		stop	: stop streaming"
	echo "		reboot	: reboot camera"
	echo "		brr	: configure camera for BroadR-Reach"
	echo "		baset	: configure camera for 100BaseT"
	exit 1
}

if [ $# -eq 0 ]; then
	usage
fi

if [ $# -gt 2 ]; then
	usage
fi


case "$1" in
start)
	CMD="-S"
	;;
stop)
	CMD="-K"
	;;
reboot)
	CMD="-R"
	;;
brr)
	CMD="-Y 1"
	;;
baset)
	CMD="-Y 0"
	;;
*)
	usage
	;;
esac

case "$#" in
1)
	salsacamctrl -i ${IP[1]} $CMD -i ${IP[2]} $CMD -i ${IP[3]} $CMD -i ${IP[4]} $CMD 
	;;
2)
	CAM=$2

	if [ "${IP[$CAM]}" = "" ]; then
		echo "Unknown camera number $CAM"
	exit 1
	fi

	salsacamctrl -i ${IP[$CAM]} $CMD
	;;
esac

