# C1 RAW Camera Hardware Acceptance

## Git

- commit: `4504b8f9ff591c6dc59c0a9e8a991c137ac38900`
- working tree: clean before this record; no source changes were made for the camera test

## Device

- stable device path: `/dev/v4l/by-id/usb-Wasintek_Wasintek_camera_00.00.01-video-index0`
- tested V4L2 node: `/dev/video0` (usb_cam cannot resolve the by-id symlink correctly and mapped it to `/dev/../../video0`)
- driver: `uvcvideo`
- card: `Wasintek camera`
- bus: `usb-0000:00:14.0-6`
- USB ID: `2aad:6373`

## V4L2 advertised modes

3840×2160@30 is advertised for both MJPG and H264. The test selected MJPG through `mjpeg2rgb`.

## ROS2 tested mode

- width: 3840
- height: 2160
- usb_cam pixel format: `mjpeg2rgb`
- ROS encoding: `rgb8`
- target FPS: 30
- topic: `/cv_camera0/image_raw`
- camera_info_url: empty parameter supplied through a temporary `/tmp` parameter file; usb_cam still logs its built-in default missing calibration path, and no calibration file was loaded
- image_proc/image_rect: not used

## Measured results

- actual width × height: 3840 × 2160
- 60s `ros2 topic hz`: final reported average 10.942 FPS (window 685); observed interval 0.048–0.587 s
- independent rclpy run: 4,831 frames over 309.818 s, average 15.587 FPS
- timestamp monotonic: PASS; 0 backward timestamps
- first timestamp: 1788190619.795457
- last timestamp: 1788190929.613024
- largest observed timestamp gap: 0.835638 s
- 5-minute continuous capture: completed approximately 310 s with no process crash or continuous stream loss; intermittent long gaps were observed

## Resource usage

Not captured reliably in this run; no CPU/RSS claim is made.

## Kernel / USB diagnostics

No matching `usb`, `uvc`, `reset`, `disconnect`, `timeout`, `bandwidth`, or `error` lines were present in the captured kernel tail.

## Saved samples

- [sample_01.png](/home/yangxuan/Autolabor-C1-ROS2/acceptance_results/camera-only-20260831_233648/sample_01.png)
- [sample_02.png](/home/yangxuan/Autolabor-C1-ROS2/acceptance_results/camera-only-20260831_233648/sample_02.png)
- [sample_03.png](/home/yangxuan/Autolabor-C1-ROS2/acceptance_results/camera-only-20260831_233648/sample_03.png)

All three files are valid 3840×2160 8-bit RGB PNGs. Pixel statistics were non-uniform and differed between samples.

## Conclusions

- Camera RAW: **PASS with degraded rate** — device identity, UVC open, resolution, image publication, monotonic timestamps and samples passed; rate and gaps require investigation.
- 4K30: **DEGRADED** — V4L2 advertises 4K30, but measured delivery was well below 30 FPS.

## Not tested

- Gimbal serial communication
- MovePantilt
- `0 -> -30 -> +30 -> 0`
- AcquireView
- `image_stamp > reached_stamp`
- Camera calibration
- image_proc/image_rect
- runtime integration
