#!/bin/bash
# Install esp32-proxy
#

set -x

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $BASEDIR
make clean
make

sudo cp esp32-proxy /var/lib/mini_pupper_bsp/
sudo cp esp32-proxy.service /lib/systemd/system/
sudo systemctl enable  esp32-proxy.service
