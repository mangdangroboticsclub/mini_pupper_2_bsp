# esp32-tests

## Getting Started

To install esp-idf use a Ubuntu 22.04 environment:

```
cd ~
git clone https://github.com/hdumcke/multipass-orchestrator-configurations.git
./multipass-orchestrator-configurations/esp-idf/build.sh 2>> .build_err.log >> .build_out.log
```

should create a working environment. I use a Raspberry Pi 4B with 2G of RAM as my build system

## Build the Project

To build the project:

```
cd ~
git clone https://github.com/hdumcke/esp32-tests
cd ~/esp32-tests/esp32
idf.py set-target esp32s3
idf.py menuconfig # you can use all default values
idf.py build
export ESPPORT=/dev/ttyUSB0
idf.py monitor # set ESP32 in download mode if you use a simple serial adapter
idf.py flash
```
