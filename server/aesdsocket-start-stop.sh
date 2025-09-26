#!/bin/sh
SERVER_NAME="aesdsocket"
SERVER_PATH="/usr/bin/aesdsocket"

case "$1" in
	start)
		echo "Trying to start server"
		start-stop-daemon -S -n $SERVER_NAME --startas $SERVER_PATH -- -d
		RET=$?
		if [ $RET -eq 0 ]; then 
			echo "$SERVER_NAME started"
		else
			echo "$SERVER_NAME started with error ${RET}" 
		       	exit 1
		fi
		;;
	stop)
		start-stop-daemon -K -n $SERVER_NAME --retry 2
		RET=$?
		if [ $RET -eq 0 ]; then
			echo "$SERVER_NAME stopped"
		else 
			echo "$SERVER_NAME stopped with error"
			exit 1
		fi
		;;
	*) 
           echo "Usage: $0 {start|stop}" 
           exit 1 
esac



