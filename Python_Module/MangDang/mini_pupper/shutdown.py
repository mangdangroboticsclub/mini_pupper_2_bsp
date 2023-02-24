import os
from threading import Timer


def shutdown():
    os.system('sudo poweroff')


class ShutDown:

    def __init__(self):
        self.timer = None
        self.delay = 3

    def request_shutdown(self):
        if self.timer is not None:
            return
        self.timer = Timer(self.delay, shutdown)
        self.timer.start()

    def cancel_shutdown(self):
        if self.timer is not None:
            self.timer.cancel()
            self.timer = None
