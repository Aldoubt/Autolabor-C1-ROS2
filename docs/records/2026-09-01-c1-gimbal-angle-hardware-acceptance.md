# C1 Gimbal Angle Hardware Acceptance

## Git

commit: `4504b8f9ff591c6dc59c0a9e8a991c137ac38900`

working tree: pre-existing untracked camera records; this acceptance added its result directory and this record.

## Device

serial path: **not resolved**

USB identity: QinHeng Electronics CH340, USB ID `1a86:7523` (visible in `lsusb`)

baud: not started

query rate: not started

## Preflight result

ROS2 package and interfaces were present. No `/dev/ttyUSB*`, `/dev/ttyACM*`, `/dev/serial/by-id/*`, or `/dev/serial/by-path/*` node was available. The CH340 was visible on USB but exposed only through the generic USB driver in sysfs, without a serial child device. The user is a member of `dialout`.

## Measurements

Not collected. The driver was not started and no serial or motion command was sent.

## Conclusion

Serial communication: **NOT TESTED / BLOCKED**

Angle feedback: **NOT TESTED**

MovePantilt: **NOT TESTED**

Repeatability: **NOT TESTED**

## Not tested

- static feedback and feedback rate
- timestamp monotonicity and protocol counters
- heading and pitch direction
- MovePantilt completion semantics
- ±30 heading sequence
- ±10/15 pitch sequence
- repeatability and return-zero spread
- ground vs encoder observations
- stabilization performance
- base disturbance rejection
- image feature stabilization
- AcquireView
- image_stamp > reached_stamp
- calibration
- image_proc/image_rect
- runtime

## Blocker

Reconnect or remap the C1 gimbal serial interface until a unique stable path such as `/dev/serial/by-id/...` and a corresponding `/dev/ttyUSB*`/`/dev/ttyACM*` node appear. Then rerun this acceptance from preflight.
