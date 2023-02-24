import netifaces as ni
from enum import Enum


class BehaviorState(Enum):
    DEACTIVATED = -1
    REST = 0
    TROT = 1
    HOP = 2
    FINISHHOP = 3
    SHUTDOWN = 96
    IP = 97
    TEST = 98
    LOWBATTERY = 99


class Display:

    def __init__(self, image_dir='/var/lib/mini_pupper_bsp'):
        self.image_dir = image_dir
        self.current_state = None
        self.log_file = '/tmp/Display.log'

    def state_to_image(self, state):
        path = "%s.png" % state.name
        return path.lower()

    def show_state(self, state):
        if state.name == self.current_state:
            return
        self.current_state = state.name
        image_path = "%s/%s" % (self.image_dir, self.state_to_image(state))
        with open(self.log_file, 'a') as fh:
            fh.write("%s\n" % image_path)

    def show_image(self, image_path):
        with open(self.log_file, 'a') as fh:
            fh.write("%s\n" % image_path)

    def show_ip(self):
        try:
            if ni.AF_INET in ni.ifaddresses('wlan0').keys():
                ip = ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr']
            elif ni.AF_INET in ni.ifaddresses('eth0').keys():
                ip = ni.ifaddresses('eth0')[ni.AF_INET][0]['addr']
            else:
                ip = 'no IPv4 address found'
        except:
            ip = 'no IPv4 address found'
        text = "IP: %s" % str(ip)
        with open(self.log_file, 'a') as fh:
            fh.write("%s\n" % text)
