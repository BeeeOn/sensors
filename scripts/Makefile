
CC=xc8
OUTPUT=-warn=0 --asmlist --opt=default,+asm,+asmfile,-speed,+space,-debug --addrqual=ignore --mode=free --outdir=/home/panda/Stažené/sen/sensors/build -ocoord.hex --runtime=default,+clear,+init,-keep,-no_startup,-download,+config,+clib,+plib --double=24 --float=24 --emi=wordwrite --output=default,-inhx032 --summary=default,-psect,-class,+mem,-hex,-file
FLAGS_END=
FLAGS_CUSTOM=	--chip=18F46J50 \
						-I/home/panda/Stažené/sen/sensors/coord/demo_board \
						-I/home/panda/Stažené/sen/sensors/lib/fitp \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/common/log \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/common/net_layer \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/common/phy_layer \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/coord/fitp \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/coord/global_storage \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/coord/link_layer \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/coord/net_layer \
						-I/opt/microchip/xc8/v1.35/include/plib \
						-I//opt/microchip/xc8/v1.35/include \
						-I/opt/microchip/xc8/v1.35/lib \
						-I/home/panda/Stažené/sen/sensors/lib/fitp/coord/phy_layer
				
SOURCE_FILES=	/home/panda/Stažené/sen/sensors/coord/main.c \
						/home/panda/Stažené/sen/sensors/coord/demo_board/hw_layer/hw.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/common/log/log.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/coord/fitp/fitp.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/coord/global_storage/global.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/coord/link_layer/link.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/coord/net_layer/net.c \
						/home/panda/Stažené/sen/sensors/lib/fitp/coord/phy_layer/phy.c
				

all:
	$(CC) $(FLAGS_CUSTOM) $(SOURCE_FILES) $(OUTPUT) $(FLAGS_END)

