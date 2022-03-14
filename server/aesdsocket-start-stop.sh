#!/bin/sh 

set -e
set -u
DAEMON_NAME=aesdsocket
DAEMON_PATH=/usr/bin/aesdsocket
DAEMON_START_OPTS=-d


case "$1" in
    start)	
        echo "starting aesdsocket daemon"
        #start a daemon
        #./aesdsocket -d
        start-stop-daemon --start --name $DAEMON_NAME -a $DAEMON_PATH -- $DAEMON_START_OPTS
        ;;
    stop)
        echo "stopping aesdsocket daemon"
        #stop a daemon
        start-stop-daemon --stop --name $DAEMON_NAME
        ;;
    *)
        echo "Usage: $0 {start} or {stop}" 
        exit 1 
esac

exit 0

