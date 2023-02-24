#!/bin/bash

set -e

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )


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
sudo apt install -y i2c-tools dpkg-dev curl python-is-python3 mpg321 python3-tk openssh-server screen
sudo sed -i "s/pulse/alsa/" /etc/libao.conf
if [ $(lsb_release -cs) == "jammy" ]; then
    sudo sed -i "s/cards.pcm.front/cards.pcm.default/" /usr/share/alsa/alsa.conf
fi

### Install LCD images
sudo rm -rf /var/lib/mini_pupper_bsp
sudo cp -r $BASEDIR/Display /var/lib/mini_pupper_bsp

### Install system components
for dir in IO_Configuration FuelGauge System esp32_proxy; do
    cd $BASEDIR/$dir
    ./install.sh
done

sudo sed -i "s|BASEDIR|$BASEDIR|" /etc/rc.local
sudo sed -i "s|BASEDIR|$BASEDIR|" /usr/bin/battery_monitor

### Install pip
cd /tmp
wget --no-check-certificate https://bootstrap.pypa.io/get-pip.py
sudo python get-pip.py

### Install Python module
sudo apt install -y python3-dev
sudo git config --global --add safe.directory $BASEDIR # temporary fix https://bugs.launchpad.net/devstack/+bug/1968798
sudo pip install $BASEDIR/Python_Module

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
