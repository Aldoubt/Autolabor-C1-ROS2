# C1 Gimbal Raw Serial Feedback Trace

## Hardware

serial: `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`

baud: 115200, 8N1

commit: `4504b8f9ff591c6dc59c0a9e8a991c137ac38900`

No formal source, parser, Action logic, tolerance, or stable-samples setting was modified.

## Acquisition

The ROS2 driver was the sole serial reader. Its `read()` calls were captured with `strace`; a second reader was intentionally not opened because competing serial readers would steal bytes and invalidate frame boundaries.

Result directory: `acceptance_results/gimbal-serial-trace-20260901_110630/`

Total capture: approximately 382 seconds. Serial `read()` calls with payload: 7,634. Bytes received: 104,936. Protocol frames reconstructed: 7,440. Valid CRC frames: 7,440. Valid angle frames (`55 12 00 ...`): 3,817. ROS samples: 601 per 60-second experiment, approximately 10 Hz. ROS header timestamps were monotonic.

The trace contains other valid 10-byte command/status responses in addition to the 18-byte angle feedback frames. The raw frame splitter used the existing protocol header and length byte; it did not reinterpret unknown frames.

## Experiment A — static starting position

The requested zero position was not independently commanded in this trace run. The device was left stationary; its actual reported ground heading changed from 30.73 to 28.17 degrees early in the capture, so this is reported as a static starting-position observation rather than a verified 0/0/0 state.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 28.230 | 0.387 | 2.560 | -1.663 | 0.039 | 0.190 |
| roll | 2.171 | 0.391 | 2.550 | -0.411 | 1.250 | 2.900 |
| pitch | 0.165 | 0.028 | 0.100 | 1.025 | 1.199 | 5.080 |

## Experiment B — pitch -10

MovePantilt returned `SUCCESS`, final feedback pitch `-10.22` degrees. During the following 60 seconds the raw encoder pitch alternated between approximately `-10.2` and `-7.7` degrees; the ground pitch also contained corresponding jumps.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 30.730 | 0.000 | 0.000 | 0.089 | 1.682 | 5.100 |
| roll | -0.689 | 1.405 | 3.170 | 0.022 | 1.749 | 5.110 |
| pitch | -10.271 | 0.432 | 5.110 | -9.205 | 1.194 | 2.550 |

## Experiment C — pitch +10

MovePantilt returned `TIMEOUT`; final feedback pitch was `+7.75` degrees. The device nonetheless produced samples near `+10.2` degrees. The raw encoder pitch alternated by approximately 2.5 degrees throughout the hold interval.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 30.730 | 0.000 | 0.000 | 1.618 | 0.889 | 2.550 |
| roll | -2.056 | 0.104 | 0.350 | 0.788 | 1.256 | 3.050 |
| pitch | 8.115 | 1.108 | 2.670 | 8.712 | 1.201 | 5.090 |

## Jump event analysis

| time | raw frame changed? | encoder changed? | ground changed? | conclusion |
|---|---|---|---|---|
| 1788232002.030 | Yes: raw encoder roll `+0.56 -> -1.97`, encoder pitch `+0.08 -> +2.55` | Yes | Mostly no | Jump already exists in raw device frame |
| 1788232154.137 | Yes: raw ground roll `+0.32 -> -2.24`, ground pitch `-10.24 -> -7.69` | Yes in nearby raw frames | Yes | Device-side frame fields change; not introduced by ROS publish |
| 1788232121.335 | Yes: raw encoder roll `+2.54 -> -0.01`, encoder pitch `-10.20 -> -7.73` | Yes | Yes in related fields | Repeated raw discontinuity at pitch -10 |
| 1788232299.442 | Yes: raw encoder pitch `+10.14 -> +7.80`, encoder roll `-0.61 -> +1.90` | Yes | Yes in related fields | Repeated raw discontinuity at pitch +10 |

For each ROS sample, the nearest raw angle frame was found within 3.7 ms maximum / 0.3 ms median. The six decoded fields matched the ROS message exactly: maximum absolute difference `0.0` degrees in all three experiments.

## Root cause judgement

Feedback jump source: **Device-side raw feedback**, with the exact internal cause unresolved between C1 firmware, encoder sampling/fusion, or IMU/attitude generation.

Protocol: **No evidence of protocol corruption**. All 7,440 reconstructed frames had valid CRC; frame headers and lengths were consistent.

ROS parser: **No evidence of a parser transformation bug in this trace**. The raw bytes, decoded using the existing parser convention, exactly matched the ROS fields. Byte order, scaling, signed conversion, and frame boundary handling did not create an additional discrepancy in the observed samples.

Ground attitude estimation: **Also contributes**. Ground values are offset from encoder values and ground pitch changes in the same raw feedback stream. This is consistent with device-side attitude/IMU estimation behavior, but raw trace alone cannot distinguish IMU fusion from firmware packaging.

Overall: **A — raw device feedback jumps; D — ground attitude also varies. Not B-only and not C-only.** The pitch actuator can reach approximately -10 and +10 in individual raw samples, but the reported stream is not continuously stable.

## Recommendation

1. Do not enter Phase 1B.2 yet. Resolve or characterize the device-side discontinuity first.
2. Do not modify the driver based on this trace alone; there is no demonstrated ROS parser mismatch.
3. Do not change MovePantilt tolerance or stable-samples yet. The current timeout behavior correctly exposes the unstable feedback instead of falsely reporting stable arrival.
4. For inspection records, preserve both `ground_*` and `encoder_*`; use encoder values for actuator position and ground values for attitude monitoring.
