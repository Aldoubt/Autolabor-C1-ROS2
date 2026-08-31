# Camera Gimbal Capability Implementation Plan

**Goal:** Add a navigation-independent ROS2 `AcquireView` capability with deterministic status/error semantics and Codex-friendly automated acceptance tooling.

**Architecture:** Keep `pantilt_camera_serial/MovePantilt` as the hardware motion primitive. Add a separate `camera_gimbal_interfaces` package for the public `AcquireView` action and health message, and a `camera_gimbal_capability` node that composes motion + stable wait + fresh-image capture. Bringup launches the capability; test tooling supports offline, simulated, and hardware acceptance.

**Tech Stack:** ROS2 Humble, rclcpp/rclpy, ROS2 Action, sensor_msgs, cv_bridge, pytest, shell acceptance runner.

## Global Constraints
- Navigation, waypoints, Nav2, map and mission sequencing are out of scope.
- Serial write success is never equivalent to motion success.
- `AcquireView` succeeds only after `MovePantilt` succeeds and a camera frame newer than the post-settle threshold is received.
- Machine-readable acceptance failures must return non-zero exit codes.
- Hardware acceptance must never report PASS if feedback/camera data is stale.

### Task 1: Freeze the public capability contract
- [ ] Add failing contract tests for `camera_gimbal_interfaces` and `AcquireView.action`.
- [ ] Add failing policy tests for goal validation, tag sanitization and timestamp freshness.
- [ ] Run tests and verify RED.
- [ ] Implement interfaces/policy and verify GREEN.

### Task 2: Implement `camera_gimbal_capability`
- [ ] Add capability node using the low-level `MovePantilt` action client and camera/status subscriptions.
- [ ] Add health publication and exact error-code mapping.
- [ ] Add fresh-image capture and optional file saving.
- [ ] Add package metadata and entry point.

### Task 3: Integrate bringup and simulation fixture
- [ ] Launch capability in real bringup.
- [ ] Add fake gimbal/camera node and fake launch for no-hardware integration acceptance.
- [ ] Add capability acceptance client with JSON artifacts and strict exit codes.

### Task 4: Add Codex + human acceptance workflow
- [ ] Add root acceptance runner for offline/simulated/hardware modes.
- [ ] Add machine-readable summary validator.
- [ ] Add Codex acceptance prompt and human checklist.
- [ ] Re-run all available local verification and package the workspace.
