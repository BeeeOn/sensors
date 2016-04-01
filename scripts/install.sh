#!/bin/sh

#simulator
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
sudo apt-get update
sudo apt-get install mosquitto -y
sudo apt-get install libmosquitto-dev -y
sudo apt-get install libmosquittopp-dev -y
sudo apt-get install mosquitto-clients -y
sudo apt-get install python-pip python-dev build-essential -y
pip install paho-mqtt
sudo apt-get install libpoco-dev -y
sudo apt-get install libglib2.0-dev -y

#xc8 depending onUbuntu
sudo apt-get install libc6:i386 libx11-6:i386 libxext6:i386 libstdc++6:i386 libexpat1:i386 -y

#SDK arm
wget http://ant-2.fit.vutbr.cz/beeeon-i686-armv7a-vfp-neon-toolchain-0.9.sh
chmod +x beeeon-i686-armv7a-vfp-neon-toolchain-0.9.sh
sudo ./beeeon-i686-armv7a-vfp-neon-toolchain-0.9.sh

#xc8 compiler
wget http://ww1.microchip.com/downloads/en/DeviceDoc/xc8-v1.35-full-install-linux-installer.run
chmod +x ./xc8-v1.35-full-install-linux-installer.run
sudo ./xc8-v1.35-full-install-linux-installer.run

#plib
#eti path: /opt/microchip/xc8/v1.35
wget --content-disposition http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en574970
chmod +x ./peripheral-libraries-for-pic18-v2.00rc3-linux-installer.run
sudo ./peripheral-libraries-for-pic18-v2.00rc3-linux-installer.run --installer-language sk --mode text --prefix /opt/microchip/xc8/v1.35/

#gui install
#sudo ./peripheral-libraries-for-pic18-v2.00rc3-linux-installer.run

#install MPLAB X IDE/IPE
wget http://www.microchip.com/mplabx-ide-linux-installer
chmod +x mplabx-ide-linux-installer
sudo ./mplabx-ide-linux-installer
