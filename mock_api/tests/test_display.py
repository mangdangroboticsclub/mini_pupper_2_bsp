import filecmp
import os
import netifaces as ni
from MangDang.mini_pupper.display import Display, BehaviorState


def test_show_state():
    os.system("rm -f /tmp/Display.log")
    disp = Display()
    disp.show_state(BehaviorState.REST)
    assert filecmp.cmp(os.path.join(os.path.dirname(__file__), 'expected_results', 'display_1'),
                       '/tmp/Display.log')


def test_show_state_twice():
    os.system("rm -f /tmp/Display.log")
    disp = Display()
    disp.show_state(BehaviorState.REST)
    disp.show_state(BehaviorState.REST)
    assert filecmp.cmp(os.path.join(os.path.dirname(__file__), 'expected_results', 'display_1'),
                       '/tmp/Display.log')


def test_show_image():
    os.system("rm -f /tmp/Display.log")
    disp = Display()
    disp.show_image("/path/to/image.png")
    assert filecmp.cmp(os.path.join(os.path.dirname(__file__), 'expected_results', 'display_2'),
                       '/tmp/Display.log')


def test_show_ip():
    os.system("rm -f /tmp/Display.log")
    disp = Display()
    disp.show_ip()
    if 'wlan0' in ni.interfaces() and ni.AF_INET in ni.ifaddresses('wlan0').keys():
        ip = ni.ifaddresses('wlan0')[ni.AF_INET][0]['addr']
        text = "IP: %s" % str(ip)
        with open('/tmp/Display.exp'.log_file, 'w') as fh:
            fh.write("%s\n" % text)
        assert filecmp.cmp('/tmp/Display.exp', '/tmp/Display.log')
    elif 'eth0' in ni.interfaces() and ni.AF_INET in ni.ifaddresses('eth0').keys():
        ip = ni.ifaddresses('eth0')[ni.AF_INET][0]['addr']
        text = "IP: %s" % str(ip)
        with open('/tmp/Display.exp'.log_file, 'w') as fh:
            fh.write("%s\n" % text)
        assert filecmp.cmp('/tmp/Display.exp', '/tmp/Display.log')
    else:
        assert filecmp.cmp(os.path.join(os.path.dirname(__file__), 'expected_results', 'display_3'),
                       '/tmp/Display.log')
