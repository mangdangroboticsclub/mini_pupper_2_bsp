# Use RPi Camera in your Mini Pupper
## Step1: Connect Pi Camera to the CM4 MIPI connector
1. Ensure that your Raspberry Pi Compute Module 4 is turned off and unplugged from any power source. To avoid damaging the ribbon cable, camera module, and connector with static electricity, be sure to discharge any static on your body before handling them.

![cable_and_camera](/imgs/camera_cable_and_camera.jpg)
2. Locate the MIPI CSI camera interface on your CM4 board. It's a small, rectangular connector with a row of metal pins inside. The connector typically has a hinged, flip-up latch that you need to open to insert the ribbon cable. This latch helps ensure the ribbon cable stays securely connected to the board during use.
![board_open](/imgs/camera_board_open.jpg)
3. Take the ribbon cable that is compatible with your Camera Module and interface on the board. 
![board_plugin](/imgs/camera_board_plugin.jpg)
Ensure that the metal contacts on the ribbon cable are facing the board bottom surface when connected to the MIPI CSI camera interface on the CM4 board. Then, carefully insert one end of the ribbon cable into the connector, ensuring that it is fully inserted and properly aligned with the pins. To insert the cable, lift up the latch using your fingers or a small tool, such as a plastic spudger or tweezers. Once the cable is fully inserted, gently push down on the latch to secure it in place.
![board_close](/imgs/camera_board_close.jpg)


4.  Take the other end of the ribbon cable and insert it into the camera module itself.
![cable_and_camera](/imgs/camera_cable_and_camera.jpg) 

The connector on the module should have a latch that can be lifted up to insert the ribbon cable. 
![camera_plugin](/imgs/camera_camera_plugin.jpg) 
Ensure that the metal contacts on the ribbon cable are aligned with the direction of the camera lens, and then insert the cable into the connector. Once the cable is inserted, lower the latch to secure it in place.
![cable_direction](/imgs/camera_cable_direction.jpg)

Remember to handle the ribbon cable and connector carefully to avoid damage, and double-check that everything is properly aligned before inserting or securing the cable.
![camera_all](/imgs/camera_all.jpg)


## Step2: Install the camera driver and configure Raspberry Pi on Mini Pupper 

```bash
cd ~/mini_pupper_2_bsp/RPiCamera  # To RPiCamera directory
```

```bash
# https://askubuntu.com/questions/1346874/failed-to-check-for-processor-microcode-upgrades-at-the-end-of-apt-get-how-ca
# fix failed-to-check-for-processor-microcode-upgrades
sudo apt-get purge needrestart
. install.sh  # Set camera configurations
reboot
```

## Step3: Test Mini Pupper MIPI camera by Ubuntu22.04 PC


- Open a terminal and do camera test
```bash
ssh -X minipupper@IP # such as, ssh -X ubuntu@192.168.5.109
cd ~/mini_pupper_2_bsp/demos  # To demos directory
python camera_ffplay_test.py  # run ffplay test demo
```
-  Camera image in RViz
TODO

