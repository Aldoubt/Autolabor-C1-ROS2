# C1 Gimbal Angle Hardware Acceptance — Follow-up

## Run

Result directory: `acceptance_results/gimbal-angle-20260901_103543/`

Serial path: `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`

## Static feedback

198 samples over approximately 20 seconds, nominally 10 Hz. `serial_connected=true`, `feedback_alive=true`, CRC errors 0, protocol errors 0. Ground heading was 38.41 degrees and stable; encoder feedback contained intermittent approximately 2.5-degree jumps in roll/pitch.

## Direction

Human observation: heading -10 degrees and return-to-zero directions were correct.

## MovePantilt

| Target | Result | Final feedback |
|---|---|---|
| heading -10 | PASS | heading -10.02 |
| heading 0 after -10 | FAIL, timeout | heading -2.18 |
| heading +10 | FAIL, timeout | heading +8.02 |
| heading 0 after +10 | PASS | heading +0.24 |
| heading -30 | PASS | heading -29.13 |
| heading 0 after -30 | PASS | heading -0.89 |
| heading +30 | PASS | heading +30.04 |
| heading 0 after +30 | PASS at tolerance edge | heading +1.45 |
| pitch -10 | FAIL, timeout | pitch -7.69 at result |
| final 0/0/0 | FAIL, timeout | heading -2.38, pitch -2.56 at result |

All successful actions showed feedback movement and stable count reaching 3 before success. No pitch +10 test and no repeatability x5 test were performed after the pitch feedback instability appeared.

## Observed issue

Feedback fields periodically jump between values such as roll 0 and approximately +/-2.5 degrees, and pitch values near the target and approximately 2.4 degrees away. These jumps prevent stable convergence with the existing 1.5-degree tolerance. No tolerance or source code was changed.

## Conclusion

Serial communication: PASS.

Angle feedback: DEGRADED — continuous frames and zero CRC/protocol errors, but intermittent roll/pitch discontinuities.

MovePantilt: DEGRADED — heading +/-30 actions reached target and waited for stable feedback, but some small-angle and pitch actions timed out because of feedback discontinuities.

Repeatability: NOT TESTED.

The driver was stopped after the final zero command attempt. Camera and RViz nodes remained running; no runtime/Nav2/SLAM was started.
