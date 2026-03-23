from MangDang.mini_pupper.ESP32Interface import ESP32Interface

import time
import os
from datetime import datetime
import matplotlib.pyplot as plt
import statistics

"""
CAUTIOUS!!! This script assume that the legs are already calibrated.

Auto diagnosis tool for mini pupper servos. This script performs systematic sweeps of each servo in both directions two times while 
recording load, voltage, and current data. The results are plotted and saved as .png files for visual analysis, 
and any servos with median load exceeding a defined threshold are flagged as potentially unhealthy.

All the parameters can be adjusted in the config section. The sweeping movement can be customized in diagnose_leg(), it is purposely written to change the sweep cycle, steps easily for testing.

Leg diagnose:
- Return the maximum and median load for each sweep and flag if median load exceeds a threshold (adjustable in code).

Plot explain:
- The top subplot shows the load readings for the servos in the same leg during the sweeps. Each servo's load is plotted in a different color, and vertical dashed lines indicate when we switch to a different servo or change sweep direction.
- The middle subplot shows the voltage readings from the board during the entire diagnosis process.
- The bottom subplot shows the current (amperage) readings from the board during the entire diagnosis process.

"""


### Configs ###
## OFFSET_STEPS = [25, 60, 60] * 4   # debug set up

OFFSET_STEPS = [110, 200, 180, 
                110, 200, 180, 
                110, 200, 180, 
                110, 200, 180]
LEG_SELECTION = [1] # leg 0-3
JOINT_SELECTION = [0, 1, 2] # joint 0-2
DWELL_TIME = 0.01
STEP_SIZE = 1
NEUTRAL_POS = [512 for _ in range(12)]
ABS_MEDIAN_CAUTIOUS_THRESHOLD = 170   
ABS_MEDIAN_DANGER_THRESHOLD = 200   

plot_name = f"bat_redmp2_wLeg_tightnesstest_tight_t1_" # for plotting file names

### Configs ###

def sweep_servo(esp32, servo_id, num_steps, leg_servo_ids, start_pos=None, step_size=1,
                dwell_time=DWELL_TIME):
    if servo_id < 0 or servo_id >= len(NEUTRAL_POS):
        raise ValueError(f"servo_id {servo_id} out of range")

    cur_positions = esp32.servos_get_position()
    positions_cmd = cur_positions.copy() if cur_positions else NEUTRAL_POS.copy()
    if start_pos is None:
        start_pos = positions_cmd[servo_id]

    positions_history = []
    load_history = {mid: [] for mid in leg_servo_ids}       # Record loads only for the servos in the same leg
    volt_history, amp_history = [], []

    for step in range(num_steps):
        pos = start_pos + step * step_size
        positions_cmd[servo_id] = pos
        esp32.servos_set_position(positions_cmd)
        time.sleep(dwell_time)

        loads = esp32.servos_get_load()
        power = esp32.get_power_status()

        positions_history.append(pos)
        for mid in leg_servo_ids:
            load_history[mid].append(loads[mid] if loads else None)

        if power:
            volt_history.append(power.get("volt"))
            amp_history.append(power.get("ampere"))
        else:
            volt_history.append(None)
            amp_history.append(None)

    positions_cmd[servo_id] = NEUTRAL_POS[servo_id]
    esp32.servos_set_position(positions_cmd)

    return {
        "servo_id": servo_id,
        "positions": positions_history,
        "loads": load_history,   # only same-leg servos
        "volts": volt_history,
        "amps": amp_history,
    }


def plot_servo_data(datasets, filename=None, title=None, dpi=150, ylimits=None):
    """
    Plot the servo load, voltage, and current data from the diagnosis sweeps.
    """
    if not datasets:
        return None

    fig, axes = plt.subplots(3, 1, figsize=(16, 10))
    all_servo_loads, all_volts, all_amps = {}, [], []

    for data in datasets:
        for mid, load_vals in data.get("loads", {}).items():
            all_servo_loads.setdefault(mid, []).extend(load_vals)
        all_volts.extend(data.get("volts", []))
        all_amps.extend(data.get("amps", []))

    for mid, load_vals in sorted(all_servo_loads.items()):
        axes[0].plot(load_vals, label=f"Servo {mid}", alpha=0.7)
    axes[1].plot(all_volts, label="Voltage")
    axes[2].plot(all_amps, label="Current")

    axes[0].set_ylabel("Load")
    axes[0].set_ylim(ylimits)
    axes[1].set_ylabel("Voltage (V)")
    axes[2].set_ylabel("Current (A)")
    axes[2].set_xlabel("Sample index")

    for ax in axes:
        ax.legend(loc="best")

    x_vlines = []
    running_len = 0
    for data in datasets:
        running_len += len(data.get("positions", []))
        if running_len > 0:
            x_vlines.append(running_len)

    for x_vline in x_vlines:
        for ax in axes:
            ax.axvline(x_vline, color='black', linestyle=':', linewidth=1, alpha=0.7)

    if title:
        fig.suptitle(title)

    out_dir = os.path.dirname(filename)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(filename, dpi=dpi)
    plt.close(fig)
    return filename

def diagnose_leg(esp32, leg_id, servo_ids, offset_steps, cautious_servos, danger_servos, file_name):
    """
    Recording the necessary data for each leg and print the data out for easy copy-paste to csv. The data includes max absolute load and median load for each sweep, which can be used to evaluate the health of the servo.
    The leg_id here is 1,2,3,4, only for display purposes.
    """

    print(f"\n=== Diagnosing leg {leg_id} (servos {servo_ids}) ===")
    datasets = []
    for servo_id, steps in zip(servo_ids, offset_steps):
        sweeps = []
        # Each block represent a full sweep in one direction and back, currently set to 2 sweeps, can be adjusted.
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=-STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=-STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=STEP_SIZE))

        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=-STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=-STEP_SIZE))
        sweeps.append(sweep_servo(esp32, servo_id, steps, servo_ids, step_size=STEP_SIZE))
        
        datasets.extend(sweeps)

        max_abs_loads, median_loads = [], []
        for sweep in sweeps:
            servo_loads = sweep["loads"][servo_id]
            max_abs = max(abs(x) for x in servo_loads)
            median_val = statistics.median(servo_loads)
            sweep["max_abs_load"] = max_abs
            sweep["median_load"] = median_val
            max_abs_loads.append(max_abs)
            median_loads.append(median_val)

        csv_values = list(map(str, max_abs_loads)) + list(map(str, median_loads)) ### formating for easy copy-paste
        csv_row = ",".join(csv_values)
        print(f"{servo_id},{csv_row}")
        
        if any(abs(m) > ABS_MEDIAN_CAUTIOUS_THRESHOLD for m in median_loads):
            print(f"Cautious!!! Servo {servo_id} load exceeds threshold {ABS_MEDIAN_CAUTIOUS_THRESHOLD}, which performance starts to degrade.")
            cautious_servos.append(servo_id)
        elif any(abs(m) > ABS_MEDIAN_DANGER_THRESHOLD for m in median_loads):
            print(f"Danger!!! Servo {servo_id} load exceeds threshold {ABS_MEDIAN_DANGER_THRESHOLD}, could affect performance significantly.")
            danger_servos.append(servo_id)


    filename = f"{file_name}leg{leg_id}_plot_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png"
    plot_servo_data(datasets, filename=filename, title=f"Leg {leg_id} Diagnostics", ylimits=(-400, 400))
    print(f"Saved plot for leg {leg_id} to {filename}")

def main():
    esp32 = ESP32Interface()
    esp32.servos_set_position(NEUTRAL_POS)
    print("Moved all servos to neutral. Starting diagnosis...")
    time.sleep(1.0)

    cautious_servos = []
    danger_servos = []
    for leg_id in LEG_SELECTION:
        servo_ids = [leg_id*3 + i for i in JOINT_SELECTION] ### only test 2, 3 motors
        offset_steps = OFFSET_STEPS[leg_id*3:(leg_id+1)*3]
        diagnose_leg(esp32, leg_id, servo_ids, offset_steps, cautious_servos, danger_servos, plot_name)

    print("\n=== Summary Report ===")
    if cautious_servos:
        print(f"Cautious servos detected, servo IDs: {cautious_servos}")
    if danger_servos:
        print(f"Danger servos detected, servo IDs: {danger_servos}")
    else:
        print("All servos passed median load threshold check.")

if __name__ == "__main__":
    main()
