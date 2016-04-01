#!/bin/bash

# Building test for iot-sensors
# @author Peter Tisovcik <xtisov00@fit.vutbr.cz>

#absolute path
ABS_PATH=`realpath -s $0`
ABS_PATH=$(dirname $ABS_PATH)
ABS_PATH=$(dirname $ABS_PATH)

#path building script
SCRIPT="${ABS_PATH}/build.sh"

#error code
ERR=0

check () {
	if [ $? -eq 0 ] ; then
		echo "OK";
	else
		echo "ERR";
		ERR=1
	fi
}

#Compile ED for hw
printf "Compile ED for hw: "
$SCRIPT -t ed -p hw >> /dev/null
check

#Compile ED for x86
printf "Compile ED for x86: "
$SCRIPT -t ed -p x86 >> /dev/null
check

#Compile COORD for hw
printf "Compile COORD for hw: "
$SCRIPT -t coord -p hw >> /dev/null
check

#Compile COORD for x86
printf "Compile COORD for x86: "
$SCRIPT -t coord -p x86 >> /dev/null
check

#Compile PAN for hw
printf "Compile PAN for hw: "
.   /usr/local/beeeon-i686/environment-setup-armv7a-vfp-neon-oe-linux-gnueabi
$SCRIPT -t pan -p hw >> /dev/null
check

#Compile PAN for x86
printf "Compile PAN for x86: "
$SCRIPT -t pan -p x86 >> /dev/null
check

exit $ERR
