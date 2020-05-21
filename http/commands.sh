#!/bin/bash

TIMEOUT=16
SERVER=progress:10000
REPORT=./report.lua

# sleep allows the client systems sockets to cool down
/usr/bin/curl -i "http://${SERVER}/newUser?username=mickey&password=goofey"
/usr/bin/curl -i "http://${SERVER}/newUser?username=mickey&password=goofey"
/usr/bin/wrk -t 4 -c 50   -s ${REPORT} -d 5  --latency "http://${SERVER}/auth?username=mickey&password=goofey"
/usr/bin/wrk -t 1 -c 1    -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 5    -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 10   -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 50   -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 100  -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 200  -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 500  -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 800  -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 1000 -s ${REPORT} -d 20 --latency "http://${SERVER}/auth?username=mickey&password=goofey"





/usr/bin/wrk -t 4 -c 50   -s ${REPORT} -d 5  --latency "http://${SERVER}/multi?multiplier=100"
/usr/bin/wrk -t 1 -c 1    -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 5    -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 10   -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 50   -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 100  -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 200  -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 500  -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 800  -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"
sleep ${TIMEOUT}
/usr/bin/wrk -t 4 -c 1000 -s ${REPORT} -d 20 --latency "http://${SERVER}/multi?multiplier=100"

