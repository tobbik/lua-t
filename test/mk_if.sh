#!/bin/sh

case "$1" in
	up)
		sudo ifconfig wlan0:1 10.128.3.201 netmask 255.255.252.0 up
		sudo ifconfig wlan0:2 10.128.3.202 netmask 255.255.252.0 up
		sudo ifconfig wlan0:3 10.128.3.203 netmask 255.255.252.0 up
		sudo ifconfig wlan0:4 10.128.3.204 netmask 255.255.252.0 up
		sudo ifconfig wlan0:5 10.128.3.205 netmask 255.255.252.0 up
		sudo ifconfig wlan0:6 10.128.3.206 netmask 255.255.252.0 up
		sudo ifconfig wlan0:7 10.128.3.207 netmask 255.255.252.0 up
		sudo ifconfig wlan0:8 10.128.3.208 netmask 255.255.252.0 up
		#sudo ifconfig enp0s3:1 192.168.0.140 netmask 255.255.252.0 up
		#sudo ifconfig enp0s3:2 192.168.0.131 netmask 255.255.252.0 up
	;;
	down)
		sudo ifconfig wlan0:1 down
		sudo ifconfig wlan0:2 down
		sudo ifconfig wlan0:3 down
		sudo ifconfig wlan0:4 down
		sudo ifconfig wlan0:5 down
		sudo ifconfig wlan0:6 down
		sudo ifconfig wlan0:7 down
		sudo ifconfig wlan0:8 down
		#sudo ifconfig enp0s3:1 down
		#sudo ifconfig enp0s3:2 down
	;;
	*)
		echo "Usage: $0 {up|down}"
esac
