#!/bin/sh

IPS="10.129.1."
IFI="wlan0"
STR=120
STP=200



case "$1" in
	up)
		c=1
		for ip in $( seq $STR 1 $STP); do
			sudo ip addr add ${IPS}${ip}/24 brd ${IPS}255 dev $IFI label ${IFI}:${c}
			c=$(($c + 1))
		done
	;;
	down)
		for ip in $( seq $STR 1 $STP); do
			sudo ip addr del ${IPS}${ip}/24 brd ${IPS}255 dev $IFI
		done
	;;
	*)
		echo "Usage: $0 {up|down}"
	;;
esac
