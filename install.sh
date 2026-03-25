#!/bin/bash

set -e

sudo apt update

# Full system upgrade is optional to keep BSP install deterministic and avoid
# interactive restart flows during provisioning on Ubuntu 24.
if [ "${MINI_PUPPER_DO_UPGRADE:-0}" = "1" ]; then
    sudo DEBIAN_FRONTEND=noninteractive apt -y upgrade
fi

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

### Write release file
echo BUILD_DATE=\"$(date)\" > ~/mini-pupper-release
echo HARDWARE=\"$(python3 $BASEDIR/Python_Module/MangDang/mini_pupper/capabilities.py)\" >> ~/mini-pupper-release
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
# Ubuntu 24.04 (Noble) uses DEB822 format in /etc/apt/sources.list.d/ubuntu.sources
# Ubuntu 22.04 (Jammy) uses traditional /etc/apt/sources.list
if [ -f /etc/apt/sources.list ] && [ -s /etc/apt/sources.list ]; then
    sudo sed -i "s/# deb-src/deb-src/g" /etc/apt/sources.list
fi

### Detect Ubuntu version once for version-specific install paths
UBUNTU_CODENAME=$(lsb_release -cs)
echo "Detected Ubuntu codename: $UBUNTU_CODENAME"

if [ "$UBUNTU_CODENAME" == "noble" ]; then
    echo "Optimizing package versions for Ubuntu 24.04 (Noble)..."
    sudo apt-get clean
    sudo apt-get install -fy || true
fi

#sudo apt update
#sudo apt -y upgrade
sudo apt install -y i2c-tools curl python-is-python3 mpg123 python3-tk openssh-server
sudo apt install -y build-essential python3-pip screen alsa-utils libportaudio2 libsndfile1
if [ "$UBUNTU_CODENAME" != "noble" ]; then
    [ ! -f "/etc/libao.conf" ] && sudo apt update && sudo apt install -y libao-common libao4
    [ -f "/etc/libao.conf" ] && sudo sed -i "s/pulse/alsa/" /etc/libao.conf
fi
if [ "$UBUNTU_CODENAME" == "jammy" ]; then
    sudo sed -i "s/cards.pcm.front/cards.pcm.default/" /usr/share/alsa/alsa.conf
fi

### Install LCD images
sudo rm -rf /var/lib/mini_pupper_bsp
sudo cp -r $BASEDIR/Display /var/lib/mini_pupper_bsp

### Install kernel headers for DKMS module compilation
KERNEL_VERSION=$(uname -r)
sudo apt install -y linux-headers-${KERNEL_VERSION} || echo "Warning: Could not install exact kernel headers, trying generic..."
if ! dpkg -l | grep -q "linux-headers-${KERNEL_VERSION}"; then
    sudo apt install -y linux-headers-raspi || true
fi

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

### Install pip and Python dependencies
# Ubuntu 24.04 enforces PEP 668 (externally-managed-environment),
# so we need --break-system-packages for system-wide pip installs.
PIP_BREAK=""
if [ "$UBUNTU_CODENAME" == "noble" ]; then
    PIP_BREAK="--break-system-packages"
fi

cd /tmp
if [ "$UBUNTU_CODENAME" == "noble" ]; then
    # Skip pip bootstrap on Noble to avoid conflicts with system-managed pip.
    sudo pip3 install $PIP_BREAK setuptools lgpio
else
    wget --no-check-certificate https://bootstrap.pypa.io/get-pip.py
    sudo python get-pip.py
    sudo pip install setuptools==58.2.0 # temporary fix https://github.com/mangdangroboticsclub/mini_pupper_ros/pull/45#discussion_r1104759104
fi
sudo pip install $PIP_BREAK sounddevice soundfile

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
    sudo PBR_VERSION=$(cd $BASEDIR; ./get-version.sh) pip install $PIP_BREAK $BASEDIR/$PYTHONMODLE
else
    sudo pip install $PIP_BREAK $BASEDIR/$PYTHONMODLE
fi

# Install Camera
MACHINE=$(uname -m)
if [ "$MACHINE" != "x86_64" ]
# install script will break virtual installation
then
~/mini_pupper_bsp/RPiCamera/install.sh
fi

### Make pwm sysfs and nvmem work for non-root users
### reference: https://github.com/raspberrypi/linux/issues/1983
### reference: https://github.com/bitula/mini_pupper-dev/blob/main/scripts/mini_pupper.sh
getent group gpio || sudo groupadd gpio && sudo gpasswd -a $(whoami) gpio
getent group dialout || sudo groupadd dialout && sudo gpasswd -a $(whoami) dialout
getent group spi || sudo groupadd spi && sudo gpasswd -a $(whoami) spi
# Pi 4 udev rule (pinctrl-bcm2711)
# Support both old kernel (gpiochip0) and kernel 6.8+ (gpiochip512)
sudo tee /etc/udev/rules.d/99-mini_pupper-gpio.rules << EOF > /dev/null
SUBSYSTEM=="gpio", ACTION=="add", ATTR{label}=="pinctrl-bcm2711", RUN+="/usr/lib/udev/gpio-mini_pupper.sh"
KERNEL=="gpiomem", OWNER="root", GROUP="gpio", MODE="0660"
EOF
sudo tee /etc/udev/rules.d/99-mini_pupper-spi.rules << EOF > /dev/null
KERNEL=="spidev0.0", OWNER="root", GROUP="spi", MODE="0660"
EOF

sudo tee /usr/lib/udev/gpio-mini_pupper.sh << 'EOF' > /dev/null
#!/bin/bash
GPIO_BASE=0
if [ -d /sys/class/gpio/gpiochip512 ]; then
    GPIO_BASE=512
fi

# Board power
PIN=$((GPIO_BASE + 21))
echo $PIN > /sys/class/gpio/export 2>/dev/null
echo out > /sys/class/gpio/gpio${PIN}/direction
chmod 666 /sys/class/gpio/gpio${PIN}/value
echo 1 > /sys/class/gpio/gpio${PIN}/value

PIN=$((GPIO_BASE + 25))
echo $PIN > /sys/class/gpio/export 2>/dev/null
echo out > /sys/class/gpio/gpio${PIN}/direction
chmod 666 /sys/class/gpio/gpio${PIN}/value
echo 1 > /sys/class/gpio/gpio${PIN}/value

# LCD power
PIN=$((GPIO_BASE + 22))
echo $PIN > /sys/class/gpio/export 2>/dev/null
echo out > /sys/class/gpio/gpio${PIN}/direction
chmod 666 /sys/class/gpio/gpio${PIN}/value
echo 1 > /sys/class/gpio/gpio${PIN}/value
EOF
sudo chmod +x /usr/lib/udev/gpio-mini_pupper.sh

sudo udevadm control --reload-rules && sudo udevadm trigger

echo 'alias esp32-cli="screen /dev/ttyUSB0 115200"' >> ~/.bashrc
