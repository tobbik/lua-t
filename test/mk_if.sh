#!/bin/sh

case "$1" in
	up)
		sudo ifconfig wlan0:1 10.128.3.200 netmask 255.255.252.0 up
		sudo ifconfig wlan0:2 10.128.3.201 netmask 255.255.252.0 up
		#sudo ifconfig enp0s3:1 192.168.0.140 netmask 255.255.252.0 up
		#sudo ifconfig enp0s3:2 192.168.0.131 netmask 255.255.252.0 up
	;;
	down)
		sudo ifconfig wlan0:1 down
		sudo ifconfig wlan0:2 down
	;;
	*)
		echo "Usage: $0 {up|down}"
esac
