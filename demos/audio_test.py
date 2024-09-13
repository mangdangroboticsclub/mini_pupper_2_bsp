#!/usr/bin/env python3
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
# Description: Mini Pupper audio record and playback script.
#
import sounddevice as sd
import soundfile as sf
import time
import os

# Audio record parameters
fs = 48000  # 48KHz,Audio sampling rate
duration = 5  # Recording duration in seconds

# Set the default speaker volume to maximum
# Headphone number is 0 without HDMI output
# Headphone number is 1 when HDMI connect the display
os.system("amixer -c 0 sset 'Headphone' 100%")

print("Mini Pupper 2 audio record start...")
record = sd.rec(int(duration * fs), samplerate=fs, channels=2)
sd.wait()  # Wait for record to finish
print("Mini Pupper 2 audio record end.")

# Increase the volume [manually]
record *= 100

# Save the record
sf.write('/tmp/mini-pupper-2-audio_test.wav', record, fs)

# Wait for 1 second to playback
time.sleep(1)

# Play audio
print("Mini Pupper 2 audio playback start...")
sd.play(record[:, 0], fs)
sd.wait()  # Wait for playback to finish
sd.play(record[:, 1], fs)
sd.wait()  # Wait for playback to finish
print("Mini Pupper 2 audio playback end.")
