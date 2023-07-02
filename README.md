# Mini Pupper: Quadrpated Robot Platform for Creativity
[MangDang](https://www.mangdang.net/) Online channel: [Discord](https://discord.gg/xJdt3dHBVw), [FaceBook](https://www.facebook.com/groups/716473723088464), [YouTube](https://www.youtube.com/channel/UCqHWYGXmnoO7VWHmENje3ug/featured), [Twitter](https://twitter.com/LeggedRobot)

Mini Pupper will make robotics easier for schools, homeschool families, enthusiasts and beyond.

- ROS: support ROS2(Humble) SLAM&Navigation robot dog at low-cost price
- OpenCV: support OpenCV official OAK-D-Lite 3D camera module and MIPI camera
- Open-source: DIY and custom what you want.
- Raspberry Pi: it’s super expandable, endorsed by Raspberry Pi.

## Overview

This repository is the BSP(board support package) for Mini Pupper 2.

## Prepare installation

### Flash Ubuntu preinstalled image to the SD card. 
- Download `ubuntu-22.04.2-preinstalled-server-arm64+raspi.img.xz` from [the official website](https://ubuntu.com/download/raspberry-pi), and flash it into your SD card.
- Put SD card into your Mini Pupper and setup your WiFi in "/etc/netplan/50-cloud-init.yaml", you can reference to [50-cloud-init.yaml](https://drive.google.com/file/d/1DN7Aa9jz5LgkGvuYcwuXQgHud_CjnDRJ/view?usp=sharing) (Please use your AP name to update "Mangdang", and password to update "mangdang" )
```
sudo vi /etc/netplan/50-cloud-init.yaml
#set your own WiFi AP and password
sudo netplan apply
```

- Do the below commands.
```
sudo apt update
sudo apt upgrade
reboot
```

## Installation

Clone this repo and install: 
```
cd ~
git clone https://github.com/mangdangroboticsclub/mini_pupper_2_bsp.git mini_pupper_bsp
cd mini_pupper_bsp
./install.sh
reboot
```

## Calibration

You can use the below command to calibrate mini pupper. 

```sh
calibrate  # this is a command
```

## Test

You can test the demos to confirm your installation, for the detailed info, please refer to the below link,
https://github.com/mangdangroboticsclub/mini_pupper_2_bsp/tree/main/demos


## License

Copyright 2023 [MangDang](https://www.mangdang.net/)

Apache-2.0 license

MangDang specializes in the research, development, and production of robot products that make peoples lives better. We are a global team with members from many countries and regions.

Many global talents are contributing to this project, you can find them on the GitHub page, we appreciate all their contribution.

If you have an interest in contributing, please check the below link and send a mail to afreez@mangdang.net
https://github.com/mangdangroboticsclub/mini_pupper_2_bsp/blob/main/CONTRIBUTING.md

All the source code are licensed under Apache-2.0 license, but NOT include the below modules.

### GPL source code in this repository
* [EEPROM](./EEPROM)
* [FuelGauge](./FuelGauge)
* [PWMController](./PWMController)
