import time
import os

import numpy as np
import matplotlib.pyplot as plt

from MangDang.mini_pupper.HardwareInterface import HardwareInterface
from MangDang.mini_pupper.ESP32Interface import ESP32Interface

"""
Script automatically sweep each joint for all legs and record the servo loads, 
to evaluate the effectiveness of the joint limits and identify any potential issues.

It could also be used as a diagnostic tool for checking the health of the servo motors or adjust the joint limits.
"""


# Initial joint angles (rad) based on previous tests, corresponding to a comfortable standing pose.
INITIAL_JOINT_ANGLES = np.array([
    [-0.00644456,  0.00644456, -0.00644456,  0.00644456],  # abduction
    [ 0.88270319,  0.88270319,  0.88270319,  0.88270319],  # hip/thigh
    [-0.69934338, -0.69934338, -0.69934338, -0.69934338],  # knee
])


def move_to_initial_pose(hw: HardwareInterface, joint_angles: np.ndarray, hold_time: float = 2.0) -> None:
    print("Moving to initial pose...")
    hw.set_actuator_postions(joint_angles)
    time.sleep(hold_time)


def sweep_axis_with_loads(
    axis_index: int,
    axis_name: str,
    hw: HardwareInterface,
    esp32: ESP32Interface,
    base_joint_angles: np.ndarray,
    delta_deg: float = 120.0,
    step_deg: float = 2.0,
    dwell: float = 0.1,
):
    """Sweep one joint axis for all legs and record servo loads.

    Pattern:
      - start from initial pose
      - sweep from initial -> +delta
      - return +delta -> initial
      - sweep initial -> -delta
      - return -delta -> initial

    Returns
    -------
    sweep_angles : list[float]
        Commanded angle (deg) for this axis.
    loads_by_leg : list[list[float]]
        Per-leg loads for this axis, using the servo IDs from PWMParams.
    """

    joint_angles = base_joint_angles.copy()

    sweep_angles: list[float] = []
    all_load_samples: list[list[float]] = []

    def record_step(current_deg: float):
        # Hardware related commands are handled here
        # Command angle to all four legs on this axis

        angle_rad = np.deg2rad(current_deg)
        joint_angles[axis_index, :] = angle_rad

        hw.set_actuator_postions(joint_angles)
        time.sleep(dwell)

        # Read all servo loads: format
        # [leg0_hip, leg0_thigh, leg0_knee, leg1_hip, leg1_thigh, leg1_knee, ...]
        load_vector = list(esp32.servos_get_load())
        sweep_angles.append(current_deg)
        all_load_samples.append(load_vector)

    start_deg = np.rad2deg(joint_angles[axis_index, 0])
    print(f"\n=== Sweeping {axis_name} (axis {axis_index}) ===")
    print(f"Start angle: {start_deg:.1f} deg")

    # Initial -> +delta
    target_pos_deg = start_deg + delta_deg
    current_deg = start_deg
    while current_deg <= target_pos_deg:
        record_step(current_deg)
        current_deg += step_deg

    # +delta -> initial
    current_deg = target_pos_deg
    while current_deg >= start_deg:
        record_step(current_deg)
        current_deg -= step_deg

    # initial -> -delta
    target_neg_deg = start_deg - delta_deg
    current_deg = start_deg
    while current_deg >= target_neg_deg:
        record_step(current_deg)
        current_deg -= step_deg

    # -delta -> initial (return sweep)
    current_deg = target_neg_deg
    while current_deg <= start_deg:
        record_step(current_deg)
        current_deg += step_deg
    
    print(f"Completed sweep for {axis_name}. Ensure back to initial pose.")
    # Return to exact initial pose at the end
    joint_angles[axis_index, :] = np.deg2rad(start_deg)
    hw.set_actuator_postions(joint_angles)
    
    return sweep_angles, all_load_samples


def plot_axis_loads(axis_name: str, sweep_angles, all_load_samples):
    """Plot loads for one movement as 3 subplots (abduction/hip/knee), 4 legs each.

    Data are plotted in sequence order with x-axis tick labels the corresponding commanded angles
    Load vector format per sample:
      [leg0_hip, leg0_thigh, leg0_knee,
       leg1_hip, leg1_thigh, leg1_knee,
       leg2_hip, leg2_thigh, leg2_knee,
       leg3_hip, leg3_thigh, leg3_knee]
    """

    steps = np.arange(len(sweep_angles))
    angles = np.asarray(sweep_angles)
    loads = np.asarray(all_load_samples)  # shape (N, 12)

    joint_names = ["Abduction", "Hip", "Knee"]
    fig, axes = plt.subplots(3, 1, sharex=True, figsize=(8, 8))

    for joint_idx, ax in enumerate(axes):
        for leg_idx in range(4):
            col = leg_idx * 3 + joint_idx
            ax.plot(steps, loads[:, col], label=f"Leg {leg_idx}")

        ax.set_ylabel(f"{joint_names[joint_idx]} load")
        ax.grid(True)
        if joint_idx == 0 and len(angles) > 0:
            angle_min = float(angles.min())
            angle_max = float(angles.max())
            ax.set_title(
                f"Loads vs commanded {axis_name} (angle range {angle_min:.1f}° to {angle_max:.1f}°)"
            )
            ax.legend()

    # Use sample index for positions, but label ticks with angles
    if len(steps) > 1:
        n_ticks = min(8, len(steps))
        tick_idx = np.linspace(0, len(steps) - 1, num=n_ticks, dtype=int)
        axes[-1].set_xticks(tick_idx)
        axes[-1].set_xticklabels([f"{angles[i]:.1f}" for i in tick_idx])

    axes[-1].set_xlabel(f"{axis_name} angle (deg)")
    fig.tight_layout()

    # Save figure in the same directory as this script
    safe_name = axis_name.lower().replace(" ", "_").replace("/", "_")
    out_dir = os.path.dirname(__file__)
    out_path = os.path.join(out_dir, f"loads_{safe_name}.png")
    fig.savefig(out_path)
    plt.close(fig)
    print(f"Saved plot for {axis_name} to {out_path}")


def main():
    # Enable joint limits
    hw = HardwareInterface(joint_checker_flag=True)
    esp32 = ESP32Interface()

    # Move robot to the default/initial pose
    move_to_initial_pose(hw, INITIAL_JOINT_ANGLES)

    # axis_index, name, delta_deg (per-joint sweep range)
    axis_plan = [
        (2, "Knee", 120.0),
        (1, "Hip", 160.0),
        (0, "Abduction", 55.0),
    ]

    # Use a dict keyed by axis name for clearer organization
    results = {}
    for axis_index, axis_name, delta_deg in axis_plan:
        sweep_angles, all_load_samples = sweep_axis_with_loads(
            axis_index=axis_index,
            axis_name=axis_name,
            hw=hw,
            esp32=esp32,
            base_joint_angles=INITIAL_JOINT_ANGLES,
            delta_deg=delta_deg,
            step_deg=2.0,
            dwell=0.1,
        )
        results[axis_name] = {
            "sweep_angles": sweep_angles,
            "loads": all_load_samples,
        }
        time.sleep(1.0)

    # Plot results for each axis
    for axis_name, data in results.items():
        plot_axis_loads(axis_name, data["sweep_angles"], data["loads"])

    print("All load plots saved in this directory.")


if __name__ == "__main__":
    main()
