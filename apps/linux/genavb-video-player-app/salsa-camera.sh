#! /bin/sh

# Use eth0:1 to avoid changing any already assigned IP, use a /30 netmask to limit risks of IP/routing conflicts with main IP address.
# Timeout wget requests after 1 try and 1 second waits.
# wget will return 1 in case of failure, 0 for success (this return code can be checked by the app calling the script).

case "$1" in
stop | start)
	sleep 1
	ifconfig eth0:1 192.168.1.1 netmask 255.255.255.252
	wget http://192.168.1.2/?stop=Stop+streaming -q -O /dev/null -t 1 -T 1
	ret=$?

	if [ $1 == "start" ]; then
		sleep 1
		wget http://192.168.1.2/?startAVB=Go+AVB+stream -q -O /dev/null -t 1 -T 1
		ret=$?
	fi

	ifconfig eth0:1 down

	exit $ret
	;;
*)
	echo "Usage: $0 start|stop" >&2
	exit 3
	;;
esac



