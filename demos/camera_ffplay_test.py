#!/usr/bin/python
import os


# Command 1: Check if ffmpeg is already installed
cmd1_check = "dpkg -s ffmpeg"
while os.system(cmd1_check) != 0:
    # Install ffmpeg
    cmd1_install = "sudo apt install -y ffmpeg"
    os.system(cmd1_install)

print("FFmpeg is already installed, continuing with the script.")

# Command 2: Check camera status
expected_result2 = "supported=1 detected=1, libcamera interfaces=0"
cmd2 = "vcgencmd get_camera"
os.system(
    "gnome-terminal --geometry=80x24+640+0 -- bash -c \
        'echo \"Expected Result: {0}\n\"; echo; {1}; echo; \
            exec bash'".format(expected_result2, cmd2))

# Command 3: List available video devices
expected_result3 = "/dev/video0"
cmd3 = "v4l2-ctl --list-devices"
os.system(
    "gnome-terminal --geometry=80x24+0+480 -- bash -c \
        'echo \"Expected Result: {0}\n\"; echo; {1}; echo; \
            exec bash'".format(expected_result3, cmd3))

# Command 4: Play video from device 0
cmd4 = "ffplay /dev/video0"
os.system(
    "gnome-terminal --geometry=80x24+640+480 -- bash -c \
        '{0}; exec bash'".format(cmd4))
