#!/bin/sh
#
# Start the GenAVB stack....
#

. "/etc/genavb/config"

case "$1" in
  start)
        if [ "$CFG_AUTO_START" -eq 1 ]; then
                avb.sh start
        fi
        ;;
  stop)
        avb.sh stop
        ;;
  *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac

exit $?

