# camera_gimbal_manager

Phase 1 provides feedback management, median filtering, stability detection, and
`/camera_gimbal/state` publication above `pantilt_camera_serial`.

The node subscribes to `/pantilt_camera_serial0/pantilt_angle_info`. Encoder values
are median-filtered with a five-sample window by default. Stability requires ten
filtered samples within 1.5 degrees of the reported target for 0.5 seconds.

This package does not send commands and does not replace or modify the hardware driver.

It also provides `/camera_gimbal/acquire_view`. The action moves through
MovePantilt, waits for settling and fresh stable feedback, then saves the first
`/cv_camera0/image_raw` frame whose timestamp is newer than the locally recorded
motion-reached timestamp. `capture_name` is used as the output filename under
the configurable `output_directory`.
