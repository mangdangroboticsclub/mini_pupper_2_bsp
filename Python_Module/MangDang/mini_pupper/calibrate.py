import curses
from MangDang.mini_pupper.ESP32Interface import ESP32Interface


def draw_menu(stdscr):
    k = 0
    leg = 0
    motor = 0
    tabs = 0
    esp32 = ESP32Interface()
    positions = [512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512]

    # Clear and refresh the screen for a blank canvas
    stdscr.clear()
    stdscr.refresh()

    # Start colors in curses
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_WHITE)

    # Loop where k is the last character pressed
    while (k != ord('q')):
        # Initialization
        stdscr.clear()
        height, width = stdscr.getmaxyx()

        if k == 9:
            tabs += 1
            tabs = tabs % 12
            leg = tabs % 4
            motor = 0
            if tabs > 3:
                motor = 1
            if tabs > 7:
                motor = 2
        elif k == curses.KEY_RIGHT:
            index = 3 * leg + motor
            positions[index] += 1
            if positions[index] > 1023:
                positions[index] = 1023
        elif k == curses.KEY_LEFT:
            index = 3 * leg + motor
            positions[index] -= 1
            if positions[index] < 0:
                positions[index] = 0
        elif k == ord('s'):
            positions = [512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512]
            esp32.save_calibration()

        esp32.servos_set_position(positions)

        # Declaration of strings
        statusbar1str = "Press tab to select motor | Press 'q' to exit | Press 's' to save"
        statusbar2str = "%s | Press left/right arrow key to adjust motor" % get_motor_name(motor, leg)

        # Rendering table
        stdscr.attron(curses.color_pair(2))
        stdscr.attron(curses.A_BOLD)
        stdscr.addstr(0, 0, "" " * 20 + Mini Pupper Calibration Tool")

        # Turning off attributes for title
        stdscr.attroff(curses.color_pair(2))
        stdscr.attroff(curses.A_BOLD)

        # Render status bar
        stdscr.attron(curses.color_pair(3))
        stdscr.addstr(height-2, 0, statusbar1str)
        stdscr.addstr(height-2, len(statusbar1str), " " * (width - len(statusbar1str) - 1))
        stdscr.addstr(height-1, 0, statusbar2str)
        stdscr.addstr(height-1, len(statusbar2str), " " * (width - len(statusbar2str) - 1))
        stdscr.attroff(curses.color_pair(3))

        stdscr.addstr(1, 0, "            front-right   front-left    back-right    back-left")
        stdscr.addstr(2, 0, "abduction")
        stdscr.addstr(3, 0, "hip")
        stdscr.addstr(4, 0, "knee")

        # Render selection
        stdscr.attron(curses.color_pair(3))
        stdscr.addstr(2 + motor, 12 + leg * 14, " " * 12)
        stdscr.attroff(curses.color_pair(3))

        # Refresh the screen
        stdscr.refresh()

        # Wait for next input
        k = stdscr.getch()


def get_motor_name(i, j):
    motor_type = {0: "abduction", 1: "hip", 2: "knee"}  # Top  # Bottom
    leg_pos = {0: "front-right", 1: "front-left", 2: "back-right", 3: "back-left"}
    final_name = motor_type[i] + " " + leg_pos[j]
    return final_name


def main():
    curses.wrapper(draw_menu)


if __name__ == "__main__":
    main()
