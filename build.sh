#!/bin/bash

# Building script for iot-sensors
# @author Peter Tisovcik <xtisov00@fit.vutbr.cz>

#absolute path
ABS_PATH=`realpath -s $0`
ABS_PATH=$(dirname $ABS_PATH)

#define constant
device_type="ed"
device_platform="hw"
BUILD_DIR="${ABS_PATH}/build"
FITP_LIB="${ABS_PATH}/lib/fitp"

#define ipe
IPE_PATH="/opt/microchip/mplabx/v3.20/mplab_ipe/ipecmd.sh"
FLASH=0

COMPILER=""
BUILD_NAME=""
FLAGS_END=""
FLAGS_CUSTOM=""
SOURCE_FILES=""
EMPTY=0
make_template=""

#parse args
parsed_args=$(getopt -o t:p:hf --long type:,platform:,help:,flash,empty -- "$@")
[ $? -eq 0 ] || {
	echo "Invalid options."
	exit 1
}

eval set -- "$parsed_args"
while true ; do
	case "$1" in
		-t|--type)
			case "$2" in
				ed|coord|pan) device_type="$2" ; shift 2 ;;
				*) echo "Invalid device type." >&2; exit 1 ;;
			esac
			;;
		-p|--platform)
			case "$2" in
				x86|hw) device_platform="$2" ; shift 2 ;;
				*) echo "Invalid device type." >&2; exit 1 ;;
			esac
			;;
		--flash)
			FLASH=1
			shift 1
			;;
		--empty)
			EMPTY=1
			shift 1
			;;
		-h|--help)
			help
			exit 0
			;;
		--) shift ; break ;;
		*) echo "Internal error of script [parsing args]" ; exit 1 ;;
	esac
done

echo -ne "\e[92m"
echo "COMPILING INFORMATION::"
echo "DEVICE TYPE: $device_type"
echo "DEVICE PLATFORM: $device_platform"

#create build folder
if [ -d "$BUILD_DIR" ] ; then
	rm -Rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"


#########################################
#Makefile for ED 						#
#########################################
if [ "$device_type" = "ed" ] ; then

	#for X86
	if [ "$device_platform" = "x86" ]  ;  then
		echo "compile ED - x86"
		COMPILER="g++"
		OUTPUT="-o $BUILD_DIR/hexa_sen"
		FLAGS_END="-lmosquittopp -DX86 -lpthread"

		FLAGS_CUSTOM='	-std=c++11 -w \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1 \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/hw_layer \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/simulator \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/senact \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/common_sim \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/ed/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/ed/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/ed/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/ed/phy_layer
				'

		SOURCE_FILES='	'"${ABS_PATH}"'/ed/x86_main-hexa_sen1.1.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/hw_layer/x86_hw.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/simulator/mqtt_iface.cc \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/simulator/simulator.c \
						'"${ABS_PATH}"'/lib/common_sim/sim_common.c \
						'"${ABS_PATH}"'/lib/fitp/common/log/x86_log.c \
						'"${ABS_PATH}"'/lib/fitp/ed/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/ed/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/ed/link_layer/link.c \
						'"${ABS_PATH}"'/lib/fitp/ed/net_layer/net.c \
						'"${ABS_PATH}"'/lib/fitp/ed/phy_layer/x86_phy.c
				'

	else
		echo "compile ED - hw"
		COMPILER="xc8"
		OUTPUT="-warn=0 --asmlist --opt=default,+asm,+asmfile,-speed,+space,-debug --addrqual=ignore --mode=free --outdir=$BUILD_DIR -oed.hex --runtime=default,+clear,+init,-keep,-no_startup,-download,+config,+clib,+plib --double=24 --float=24 --emi=wordwrite --output=default,-inhx032 --summary=default,-psect,-class,+mem,-hex,-file"
		FLAGS_END=""

		FLAGS_CUSTOM='	--chip=18F46J50 \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1 \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/ds18b20 \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/edid_config \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/hw_layer \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/senact \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/sht21lib \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/spieeprom \
						-I'"${ABS_PATH}"'/ed/hexa_sen1.1/vibra \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/ed/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/ed/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/ed/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/ed/net_layer \
						-I/opt/microchip/xc8/v1.35/include/plib \
						-I//opt/microchip/xc8/v1.35/include \
						-I/opt/microchip/xc8/v1.35/lib \
						-I'"${ABS_PATH}"'/lib/fitp/ed/phy_layer
				'

		SOURCE_FILES='	'"${ABS_PATH}"'/ed/main-hexa_sen1.1.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/ds18b20/ds18b20.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/edid_config/edid_config.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/hw_layer/hw.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/senact/senact.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/sht21lib/sht21lib.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/spieeprom/spieeprom.c \
						'"${ABS_PATH}"'/ed/hexa_sen1.1/vibra/vibra.c \
						'"${ABS_PATH}"'/lib/fitp/common/log/log.c \
						'"${ABS_PATH}"'/lib/fitp/ed/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/ed/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/ed/link_layer/link.c \
						'"${ABS_PATH}"'/lib/fitp/ed/net_layer/net.c \
						'"${ABS_PATH}"'/lib/fitp/ed/phy_layer/phy.c
				'
	fi
fi

#########################################
#Makefile for COORD						#
#########################################
if [ "$device_type" = "coord" ] ; then

	#for X86
	if [ "$device_platform" = "x86" ]  ;  then
		echo "compile COORD - x86"
		COMPILER="g++"
		OUTPUT="-o $BUILD_DIR/coord"
		FLAGS_END="-lmosquittopp -DX86 -lpthread"

		FLAGS_CUSTOM='	-std=c++11 -w \
						-I'"${ABS_PATH}"'/coord/demo_board \
						-I'"${ABS_PATH}"'/coord/demo_board/simulator \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/common_sim \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/coord/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/coord/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/coord/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/coord/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/coord/phy_layer
				'

		SOURCE_FILES='	'"${ABS_PATH}"'/coord/x86_main.c \
						'"${ABS_PATH}"'/coord/demo_board/hw_layer/x86_hw.c \
						'"${ABS_PATH}"'/coord/demo_board/simulator/mqtt_iface.cc \
						'"${ABS_PATH}"'/coord/demo_board/simulator/simulator.c \
						'"${ABS_PATH}"'/lib/common_sim/sim_common.c \
						'"${ABS_PATH}"'/lib/fitp/common/log/x86_log.c \
						'"${ABS_PATH}"'/lib/fitp/coord/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/coord/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/coord/link_layer/link.c \
						'"${ABS_PATH}"'/lib/fitp/coord/net_layer/net.c \
						'"${ABS_PATH}"'/lib/fitp/coord/phy_layer/x86_phy.c
				'

	else
		echo "compile COORD - hw"
		COMPILER="xc8"
		OUTPUT="-warn=0 --asmlist --opt=default,+asm,+asmfile,-speed,+space,-debug --addrqual=ignore --mode=free --outdir=$BUILD_DIR -ocoord.hex --runtime=default,+clear,+init,-keep,-no_startup,-download,+config,+clib,+plib --double=24 --float=24 --emi=wordwrite --output=default,-inhx032 --summary=default,-psect,-class,+mem,-hex,-file"
		FLAGS_END=""

		FLAGS_CUSTOM='	--chip=18F46J50 \
						-I'"${ABS_PATH}"'/coord/demo_board \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/coord/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/coord/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/coord/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/coord/net_layer \
						-I/opt/microchip/xc8/v1.35/include/plib \
						-I//opt/microchip/xc8/v1.35/include \
						-I/opt/microchip/xc8/v1.35/lib \
						-I'"${ABS_PATH}"'/lib/fitp/coord/phy_layer
				'

		SOURCE_FILES='	'"${ABS_PATH}"'/coord/main.c \
						'"${ABS_PATH}"'/coord/demo_board/hw_layer/hw.c \
						'"${ABS_PATH}"'/lib/fitp/common/log/log.c \
						'"${ABS_PATH}"'/lib/fitp/coord/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/coord/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/coord/link_layer/link.c \
						'"${ABS_PATH}"'/lib/fitp/coord/net_layer/net.c \
						'"${ABS_PATH}"'/lib/fitp/coord/phy_layer/phy.c
				'
	fi
fi



#########################################
#Makefile for PAN						#
#########################################
if [ "$device_type" = "pan" ] ; then

	#for X86
	if [ "$device_platform" = "x86" ]  ;  then
		echo "compile PAN - x86"

		COMPILER="g++"
		OUTPUT="-o $BUILD_DIR/pan"
		FLAGS_END="-IPocoXML  -lPocoFoundation -lPocoUtil -lPocoNet -lmosquittopp  -lpthread ` pkg-config glib-2.0  --cflags --libs` -pthread -DX86"

		FLAGS_CUSTOM='	-Wall -pedantic -std=c++11 -w \
						-I'"${ABS_PATH}"'/pan \
						-I'"${ABS_PATH}"'/pan/olimex_lime   \
						-I'"${ABS_PATH}"'/pan/olimex_lime/mosquitto \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/common_sim \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/pan/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/pan/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/pan/net_layer
			'

		SOURCE_FILES='	'"${ABS_PATH}"'/pan/x86_main.cc \
						'"${ABS_PATH}"'/pan/olimex_lime/mosquitto/x86_mqtt_iface.cc \
						'"${ABS_PATH}"'/lib/common_sim/sim_common.c \
						'"${ABS_PATH}"'/pan/olimex_lime/phy_layer/x86_phy.cc \
						'"${ABS_PATH}"'/pan/olimex_lime/simulator/simulator.c \
						'"${ABS_PATH}"'/lib/fitp/pan/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/pan/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/pan/link_layer/link.cc \
						'"${ABS_PATH}"'/lib/fitp/pan/net_layer/net.cc \
			'
	else
		echo "compile PAN - hw"

		COMPILER="arm-oe-linux-gnueabi-g++ -march=armv7-a -mthumb-interwork -mfloat-abi=softfp -mfpu=neon --sysroot=/usr/local/beeeon-i686/sysroots/armv7a-vfp-neon-oe-linux-gnueabi"
		OUTPUT="-o $BUILD_DIR/fitprotocold"
		FLAGS_END="-IPocoXML -lPocoFoundation -lPocoUtil -lPocoNet -lmosquittopp  -lpthread ` pkg-config glib-2.0  --cflags --libs`"

		FLAGS_CUSTOM='	-Wall -pedantic -std=c++11 -w \
						-I'"${ABS_PATH}"'/pan \
						-I'"${ABS_PATH}"'/pan/olimex_lime   \
						-I'"${ABS_PATH}"'/pan/olimex_lime/mosquitto \
						-I'"${ABS_PATH}"'/lib/fitp \
						-I'"${ABS_PATH}"'/lib/fitp/common/log \
						-I'"${ABS_PATH}"'/lib/fitp/common/net_layer \
						-I'"${ABS_PATH}"'/lib/fitp/common/phy_layer \
						-I'"${ABS_PATH}"'/lib/fitp/pan/global_storage \
						-I'"${ABS_PATH}"'/lib/fitp/pan/link_layer \
						-I'"${ABS_PATH}"'/lib/fitp/pan/net_layer \

			'

		SOURCE_FILES='	'"${ABS_PATH}"'/pan/main.cc \
						'"${ABS_PATH}"'/pan/olimex_lime/mosquitto/mqtt_iface.cc \
						'"${ABS_PATH}"'/pan/olimex_lime/phy_layer/phy.cc \
						'"${ABS_PATH}"'/lib/fitp/pan/fitp/fitp.c \
						'"${ABS_PATH}"'/lib/fitp/pan/global_storage/global.c \
						'"${ABS_PATH}"'/lib/fitp/pan/link_layer/link.cc \
						'"${ABS_PATH}"'/lib/fitp/pan/net_layer/net.cc \
			'
	fi
fi

echo "-------------------------------------------------------------"
echo -n -e "\e[39m" #reset farby
#make template
make_template='
all:
	$(CC) $(FLAGS_CUSTOM) $(SOURCE_FILES) $(OUTPUT) $(FLAGS_END)
'

#data for make template
make_template="
CC=$COMPILER
OUTPUT=$OUTPUT
FLAGS_END=$FLAGS_END
FLAGS_CUSTOM=$FLAGS_CUSTOM
SOURCE_FILES=$SOURCE_FILES
$make_template"

#create make template
echo "$make_template" > "Makefile"

if [ $EMPTY -eq 1 ] ; then
	make >> /dev/null
else
	make
fi
ERR=$?
rm ${ABS_PATH}/Makefile
rm -f arn=0.cof

#flash mcu
if [ $FLASH -eq 1 ] ; then
	$IPE_PATH -TPICD3 -W3.3 -MY -P18F46J50 -F"${BUILD_DIR}/${device_type}.hex"

	#clean tmp file
	rm log.0
	rm MPLABXLog.xml*
fi

exit $ERR
