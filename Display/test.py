from enum import Enum
from MangDang.mini_pupper.display import Display


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


def main():
    """ Display a test picture
    """
    disp = Display()
    disp.show_state(BehaviorState.TEST)


main()
