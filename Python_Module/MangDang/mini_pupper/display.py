import netifaces as ni
from PIL import Image, ImageDraw, ImageFont
from threading import Thread
from enum import Enum
from MangDang.LCD.ST7789 import ST7789


def show_image(image):

    # init st7789 device
    disp = ST7789()
    disp.begin()
    disp.clear()

    # show picture
    disp.display(image)


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
        self.thread = None
        self.current_state = None

    def state_to_image(self, state):
        path = "%s.png" % state.name
        return path.lower()

    def show_state(self, state):
        if state.name == self.current_state:
            return
        self.current_state = state.name
        image_path = "%s/%s" % (self.image_dir, self.state_to_image(state))
        self.show_image(image_path)

    def show_image(self, image_path):
        image = Image.open(image_path)
        image.resize((320, 240))
        self.thread = Thread(target=show_image, args=(image,))
        self.thread.start()

    def show_ip(self):
        image_path = "%s/%s" % (self.image_dir, self.state_to_image(BehaviorState.IP))
        image = Image.open(image_path)
        image.resize((320, 240))
        if ni.AF_INET in ni.ifaddresses('wlan0').keys():
            ip = ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr']
        elif ni.AF_INET in ni.ifaddresses('eth0').keys():
            ip = ni.ifaddresses('eth0')[ni.AF_INET][0]['addr']
        else:
            ip = 'no IPv4 address found'
        font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 30)
        draw = ImageDraw.Draw(image)
        text = "IP: %s" % str(ip)
        draw.text((20, 95), text, font=font, fill="#000000", spacing=0, align='left')
        self.thread = Thread(target=show_image, args=(image,))
        self.thread.start()
