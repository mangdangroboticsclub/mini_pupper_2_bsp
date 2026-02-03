#!/bin/bash

if [ -f /boot/firmware/reconfigure ]
then
    sudo cloud-init clean
    sudo rm /etc/netplan/*
    sudo rm /boot/firmware/reconfigure
    sudo reboot
fi
/home/ubuntu/StanfordQuadruped/configure_network.sh
