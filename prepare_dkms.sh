#!/bin/bash

set -e

### Get directory where this script is installed
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

sudo apt-get install -y dkms

cd $BASEDIR/rpi_i2s_audio
sudo mkdir -p /usr/src/rpi_i2s_audio-1.0
sudo cp Makefile /usr/src/rpi_i2s_audio-1.0
sudo cp rpi_i2s_audio.c /usr/src/rpi_i2s_audio-1.0/
sudo cp dkms.conf /usr/src/rpi_i2s_audio-1.0/

sudo dkms add -m rpi_i2s_audio -v 1.0
sudo dkms build -m rpi_i2s_audio -v 1.0
sudo dkms install -m rpi_i2s_audio -v 1.0
