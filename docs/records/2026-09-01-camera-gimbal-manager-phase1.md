# camera_gimbal_manager Phase 1

## Scope

Added an independent C++ ROS 2 Humble capability layer above
`pantilt_camera_serial`. The phase is limited to feedback management, median
filtering, stability detection, and state publication. It does not modify the
driver, protocol, `MovePantilt`, or runtime.

## Package and interface

- Added `src/camera_gimbal_manager`.
- Added `camera_gimbal_interfaces/msg/GimbalState.msg`.
- Subscribed to `/pantilt_camera_serial0/pantilt_angle_info`.
- Published `/camera_gimbal/state`.

## Configuration

`config/gimbal_manager.yaml` defaults to a median window of 5, an arrival window
of 10, 1.5 degree tolerance, and 0.5 second stable duration.

## Tests

Unit coverage includes the specified median-filter jump rejection, a stable
sequence, and an alternating jump sequence. Build/test results are recorded
after running `colcon build` and `colcon test`.

On 2026-09-01 in the ROS 2 Humble environment:

- `colcon build`: passed, 6 packages finished.
- `colcon test`: passed for `camera_gimbal_manager` (3/3 tests); the existing
  Python package reported no tests.

## AcquireView continuation

`camera_gimbal_interfaces/action/AcquireView.action` now exposes the requested
`capture_name`, capture timestamp, encoder/result angles, and feedback progress
fields while retaining the existing fields for compatibility with the already
present capability package. The manager action server calls
`/pantilt_camera_serial0/move_pantilt`, settles, requires fresh stable feedback,
then accepts only an image newer than the locally recorded motion-reached time.

The repository's existing fake gimbal/camera fixture publishes both fake
feedback and images and can be used for end-to-end exercising; the manager's
automated C++ tests currently cover the estimator and stability primitives.
