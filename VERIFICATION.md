# Verification status

Checked on 2026-08-29 in the ChatGPT execution environment.

## Passed in this environment

Fresh verification output is stored in `verification_local.log`.

- Python/source contract regression: `33 passed`.
- Pure C++ arrival/stability judge test: PASS with `-Wall -Wextra -Werror -pedantic`.
- Pure C++ serial protocol compatibility/safety test: PASS with `-Wall -Wextra -Werror -pedantic`.
- Python syntax compilation: 14 files PASS.
- XML parsing: 6 files PASS.
- YAML/RViz parsing: 3 files PASS.
- ROS2 launch Python AST parsing: 3 files PASS.
- `tools/run_acceptance.sh`: `bash -n` PASS.
- Active source scan: no catkin/rospy/ROS1 runtime/build commands found.
- Executable permissions: acceptance tools and bringup scripts PASS.
- `camera_gimbal_capability` setuptools metadata: package name/version resolves to `camera_gimbal_capability 1.0.0`.

The source tests include contracts for the public `AcquireView` interface, strict fresh-image semantics, mechanical/parameter validation, deterministic fake hardware, camera/gimbal failure gates, post-motion image timeout, cancellation propagation, and prevention of a late cancellation being overwritten by SUCCESS.

## Not verified in this environment

This container does **not** contain:

- `/opt/ros/humble/setup.bash`
- `ros2`
- `colcon`
- the physical C1 camera/gimbal

Therefore this document does **not** claim a ROS2 Humble `colcon build`, ROS graph integration test, real serial protocol timing, encoder behavior, 3840×2160@30 camera operation, RViz2 plugin runtime load, or physical acceptance has passed.

Those are deliberately left to the target Ubuntu 22.04 + ROS2 Humble machine and the Codex + human workflow.

## Target-machine acceptance sequence

Run from the workspace root:

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y

./tools/run_acceptance.sh offline
./tools/run_acceptance.sh simulated
```

Only after both gates PASS and a person confirms the device is mechanically safe to move:

```bash
CAMERA_DEVICE=/dev/video4 \
GIMBAL_PORT=/dev/ttyUSB0 \
./tools/run_acceptance.sh hardware
```

After `MACHINE PASS`, complete `HUMAN_ACCEPTANCE_CHECKLIST.md` before declaring the module accepted.

If Codex changes any code while fixing a failure, restart acceptance from `offline -> simulated`; do not reuse earlier PASS results as evidence for the modified tree.
