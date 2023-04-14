#!/bin/bash

set -e

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

sudo apt-get install -y dkms

cd $BASEDIR/rpi-i2s-audio
sudo mkdir -p /usr/src/rpi-i2s-audio-1.0
sudo cp Makefile /usr/src/rpi-i2s-audio-1.0
sudo cp rpi-i2s-audio.c /usr/src/rpi-i2s-audio-1.0/
sudo cp dkms.conf /usr/src/rpi-i2s-audio-1.0/

sudo dkms add -m rpi-i2s-audio -v 1.0
sudo dkms build -m rpi-i2s-audio -v 1.0
sudo dkms install -m rpi-i2s-audio -v 1.0
