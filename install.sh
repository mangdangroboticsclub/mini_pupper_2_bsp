#!/bin/bash

set -e

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

### Write release file
echo BUILD_DATE=\"$(date)\" > ~/mini-pupper-release
echo HARDWARE=\"$(python3 $BASEDIR/Python_Module/MangDang/mini_pupper/capabilities.py)\" > ~/mini-pupper-release
echo MACHINE=\"$(uname -m)\" >> ~/mini-pupper-release
if [ -f /boot/firmware/user-data ]
then
    echo CLOUD_INIT_CLONE=\"$(grep clone /boot/firmware/user-data | awk -F'"' '{print $2}')\" >> ~/mini-pupper-release
    echo CLOUD_INIT_SCRIPT=\"$(grep setup_out /boot/firmware/user-data | awk -F'"' '{print $2}')\" >> ~/mini-pupper-release
else
    echo BUILD_SCRIPT=\"$(cd ~; ls *build.sh)\" >> ~/mini-pupper-release
fi
echo BSP_VERSION=\"$(cd ~/mini_pupper_bsp; ./get-version.sh)\" >> ~/mini-pupper-release
cd ~/mini_pupper_bsp
TAG_COMMIT=$(git rev-list --abbrev-commit --tags --max-count=1)
TAG=$(git describe --abbrev=0 --tags ${TAG_COMMIT} 2>/dev/null || true)
BSP_VERSION=$(./get-version.sh)
if [ "v$BSP_VERSION" == "$TAG" ]
then
    echo IS_RELEASE=YES >> ~/mini-pupper-release
else
    echo IS_RELEASE=NO >> ~/mini-pupper-release
fi

source  ~/mini-pupper-release

############################################
# wait until unattended-upgrade has finished
############################################
tmp=$(ps aux | grep unattended-upgrade | grep -v unattended-upgrade-shutdown | grep python | wc -l)
[ $tmp == "0" ] || echo "waiting for unattended-upgrade to finish"
while [ $tmp != "0" ];do
sleep 10;
echo -n "."
tmp=$(ps aux | grep unattended-upgrade | grep -v unattended-upgrade-shutdown | grep python | wc -l)
done

### Give a meaningfull hostname
grep -q "mini_pupper_v2" /etc/hostname || echo "mini_pupper_v2" | sudo tee /etc/hostname
grep -q "mini_pupper_v2" /etc/hosts || echo "127.0.0.1	mini_pupper_v2" | sudo tee -a /etc/hosts


### upgrade Ubuntu and install required packages
echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections
sudo sed -i "s/# deb-src/deb-src/g" /etc/apt/sources.list
sudo apt update
sudo apt -y upgrade
sudo apt install -y i2c-tools dpkg-dev curl python-is-python3 mpg321 python3-tk openssh-server screen alsa-utils libportaudio2 libsndfile1
sudo sed -i "s/pulse/alsa/" /etc/libao.conf
if [ $(lsb_release -cs) == "jammy" ]; then
    sudo sed -i "s/cards.pcm.front/cards.pcm.default/" /usr/share/alsa/alsa.conf
fi

### Install LCD images
sudo rm -rf /var/lib/mini_pupper_bsp
sudo cp -r $BASEDIR/Display /var/lib/mini_pupper_bsp

### Install system components
$BASEDIR/prepare_dkms.sh
if [ "$MACHINE" == "x86_64" ]
then
    COMPONENTS=(FuelGauge System esp32_proxy rpi-i2s-audio)
else
    COMPONENTS=(IO_Configuration FuelGauge System esp32_proxy rpi-i2s-audio)
fi
for dir in ${COMPONENTS[@]}; do
    cd $BASEDIR/$dir
    ./install.sh
done

### Install pip
cd /tmp
wget --no-check-certificate https://bootstrap.pypa.io/get-pip.py
sudo python get-pip.py
sudo pip install setuptools==58.2.0 # temporary fix https://github.com/mangdangroboticsclub/mini_pupper_ros/pull/45#discussion_r1104759104
sudo pip install sounddevice soundfile

### Install Python module
sudo apt install -y python3-dev
sudo git config --global --add safe.directory $BASEDIR # temporary fix https://bugs.launchpad.net/devstack/+bug/1968798
if [ "$MACHINE" == "x86_64" ]
then
    PYTHONMODLE=mock_api
else
    PYTHONMODLE=Python_Module
fi
if [ "$IS_RELEASE" == "YES" ]
then
    sudo PBR_VERSION=$(cd $BASEDIR; ./get-version.sh) pip install $BASEDIR/$PYTHONMODLE
else
    sudo pip install $BASEDIR/$PYTHONMODLE
fi

### Make pwm sysfs and nvmem work for non-root users
### reference: https://github.com/raspberrypi/linux/issues/1983
### reference: https://github.com/bitula/mini_pupper-dev/blob/main/scripts/mini_pupper.sh
getent group gpio || sudo groupadd gpio && sudo gpasswd -a $(whoami) gpio
getent group dialout || sudo groupadd dialout && sudo gpasswd -a $(whoami) dialout
getent group spi || sudo groupadd spi && sudo gpasswd -a $(whoami) spi
sudo tee /etc/udev/rules.d/99-mini_pupper-gpio.rules << EOF > /dev/null
KERNELS=="gpiochip0", SUBSYSTEM=="gpio", ACTION=="add", ATTR{label}=="pinctrl-bcm2711", RUN+="/usr/lib/udev/gpio-mini_pupper.sh"
KERNEL=="gpiomem", OWNER="root", GROUP="gpio", MODE="0660"
EOF
sudo tee /etc/udev/rules.d/99-mini_pupper-spi.rules << EOF > /dev/null
KERNEL=="spidev0.0", OWNER="root", GROUP="spi", MODE="0660"
EOF

sudo tee /usr/lib/udev/gpio-mini_pupper.sh << EOF > /dev/null
#!/bin/bash
# Board power
echo 21 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio21/direction
chmod 666 /sys/class/gpio/gpio21/value
echo 1 > /sys/class/gpio/gpio21/value

echo 25 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio25/direction
chmod 666 /sys/class/gpio/gpio25/value
echo 1 > /sys/class/gpio/gpio25/value

# LCD power
echo 22 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio22/direction
chmod 666 /sys/class/gpio/gpio22/value
echo 1 > /sys/class/gpio/gpio22/value
EOF
sudo chmod +x /usr/lib/udev/gpio-mini_pupper.sh

sudo udevadm control --reload-rules && sudo udevadm trigger

echo 'alias esp32-cli="screen /dev/ttyUSB0 115200"' >> ~/.bashrc
