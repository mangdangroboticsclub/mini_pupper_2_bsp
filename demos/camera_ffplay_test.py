#!/usr/bin/python
#
# Copyright 2023 MangDang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Herman Ye @MangDang
# Description:
# This python script is used to test the MIPI camera module on Raspberry Pi.
# Please refer to the below guide to do the test.
#    https://github.com/mangdangroboticsclub/mini_pupper_2_bsp/tree/main/RPiCamera
#
# Attention: you must use "ssh -X" on your Ubuntu22.04 PC to connect your Mini Pupper device.
#

import os
import subprocess


def run_command(command):
    process = subprocess.Popen(
        command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    output, error = process.communicate()
    return output.decode('utf-8').strip(), error.decode('utf-8').strip()


# Set log files
current_directory = os.path.dirname(os.path.abspath(__file__))
log_out_file = os.path.join(current_directory, "log_out_camera_install.txt")
log_err_file = os.path.join(current_directory, "log_err_camera_install.txt")

# Command 1: Check if ffmpeg is already installed
cmd1_check = "dpkg -s ffmpeg"
output, error = run_command(cmd1_check)
while "Status: install ok installed" not in output:
    # Install ffmpeg
    cmd1_install = "sudo apt install -y ffmpeg"
    os.system(cmd1_install)
    output, error = run_command(cmd1_check)

with open(log_out_file, 'a') as log_out, open(log_err_file, 'a') as log_err:
    log_out.write(output + "\n")
    log_err.write(error + "\n")

print("FFmpeg is already installed, continuing with the script.")

# Command 2: Check camera status
expected_result2 = "supported=1 detected=1, libcamera interfaces=0"
cmd2 = "vcgencmd get_camera"
output, error = run_command(cmd2)

with open(log_out_file, 'a') as log_out, open(log_err_file, 'a') as log_err:
    log_out.write(output + "\n")
    log_err.write(error + "\n")

print("Expected Result: {}\n".format(expected_result2))
print(output)

# Command 3: List available video devices
expected_result3 = "/dev/video0"
cmd3 = "v4l2-ctl --list-devices"
output, error = run_command(cmd3)

with open(log_out_file, 'a') as log_out, open(log_err_file, 'a') as log_err:
    log_out.write(output + "\n")
    log_err.write(error + "\n")

print("Expected Result: {}\n".format(expected_result3))
print(output)

# Command 4: Play video from device 0 with SSH
# ssh -X user@ip_address NEED on PC!
cmd4 = "ffplay -vf 'scale=320:240' -an -b:v 500k /dev/video0"
os.system(cmd4)

print("\nCamera Test Finished!\n\
    Please run command below to check the log files for more details:\n\
        Output log file:\nless {}\n\
        Error log file:\nless{}".format(log_out_file, log_err_file))
