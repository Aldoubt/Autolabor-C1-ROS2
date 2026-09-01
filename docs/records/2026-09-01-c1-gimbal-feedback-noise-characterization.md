# C1 Gimbal Feedback Noise Characterization

## Hardware

serial: `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`

baud: 115200

query rate: 10 Hz

commit: `4504b8f9ff591c6dc59c0a9e8a991c137ac38900`

No source, tolerance, stable_samples, Action logic, protocol, or parser was modified.

## Acquisition

All four CSV files were sampled at approximately 10.000 Hz. Header timestamps were monotonic in every file. CRC and protocol counters during the run remained zero. The driver was stopped at the end; no runtime/Nav2/SLAM/AcquireView was started.

## Experiment A — 0/0/0 static

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 40.970 | 0.000 | 0.000 | -0.444 | 0.222 | 0.770 |
| roll | 1.010 | 0.019 | 0.080 | 0.269 | 0.055 | 0.410 |
| pitch | -2.743 | 0.096 | 0.330 | -0.306 | 0.073 | 0.400 |

593 samples over 59.20 seconds, 10.000 Hz. Ground values were very stable. Encoder values were also stable at zero, with small heading variation.

## Experiment B — heading +30 static

Action result: timeout because stable samples were repeatedly broken by feedback jumps. Final Action feedback was heading 28.53 degrees. The gimbal was held there for the 60-second acquisition.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 74.180 | 0.417 | 2.560 | 29.676 | 0.264 | 2.750 |
| roll | 0.127 | 0.406 | 2.550 | 0.357 | 0.261 | 2.540 |
| pitch | -4.010 | 0.247 | 0.840 | -1.469 | 0.095 | 0.560 |

The encoder heading median was 29.68 degrees, close to the commanded position. Both ground and encoder streams show periodic outliers in the other axes.

## Experiment C — pitch -10 static

Action result: success; final feedback was heading 0.36, roll 1.00, pitch -10.21 degrees.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 43.530 | 0.000 | 0.000 | 0.491 | 0.160 | 2.740 |
| roll | -1.256 | 0.218 | 0.870 | -0.651 | 1.251 | 5.100 |
| pitch | -8.510 | 0.039 | 0.150 | -9.307 | 1.166 | 2.550 |

The encoder pitch median was -10.13 degrees (nominally at target), while the mean was pulled toward -9.31 by the periodic alternate values. Ground pitch was stable near -8.51 degrees but offset from the encoder/target value.

## Experiment D — pitch +10 static

Action result: success; final feedback was heading -0.53, roll -0.02, pitch 10.18 degrees.

| axis | ground mean | ground std | ground range | encoder mean | encoder std | encoder range |
|---|---:|---:|---:|---:|---:|---:|
| heading | 43.530 | 0.000 | 0.000 | -0.019 | 1.678 | 5.110 |
| roll | -1.490 | 0.016 | 0.080 | 0.153 | 1.678 | 5.110 |
| pitch | 8.839 | 1.440 | 3.340 | 8.601 | 1.159 | 2.550 |

The encoder pitch median was 7.79 degrees and the ground pitch median was 9.79 degrees. Both streams contain significant variation in this condition; the Action final sample was nevertheless near +10 degrees.

## Answers

### 1. Which angle source is more stable?

At static zero, both sources are stable. Across commanded positions, ground heading/pitch often has a lower standard deviation, but the ground values are not the commanded actuator angle. Encoder heading tracks the command directly (approximately 29.68 for +30), while encoder roll/pitch contain the periodic approximately 2.5-degree discontinuities. Therefore stability and semantic correctness are separate: ground is useful for attitude monitoring; encoder is the direct actuator-position source.

### 2. Source of the pitch problem

Conclusion: **both, with different symptoms**.

The actuator/encoder can reach the requested pitch: the -10 Action final was -10.21 and its encoder median was -10.13; +10 similarly reached +10.18 at the final sample. This argues against a simple permanent mechanical inability to reach pitch.

However, the encoder pitch stream is not continuously stable: it alternates by roughly 2.5 degrees, and +10 has a broad multi-degree spread. Ground pitch is offset from the encoder/target (about -8.51 versus -10.13 at -10) and is also less reliable at +10. Thus the evidence supports feedback discontinuity/ground-estimation variation plus a smaller residual position/offset component, rather than only mechanical non-arrival.

### 3. MovePantilt follow-up suggestions

No implementation was made. Suggestions for a later decision are:

- retain the current 1.5-degree tolerance as the acceptance baseline;
- investigate the periodic feedback discontinuity before changing tolerance or stable_samples;
- consider encoder angles for actuator arrival judgment, with an explicitly documented policy;
- use ground angles for attitude monitoring, not as a substitute for actuator position.

### 4. Inspection-system recording recommendation

Recommend recording **ground + encoder together**. Use `encoder_*` as the commanded gimbal/actuator position and `heading/roll/pitch` as the measured ground-referenced attitude. Keeping both preserves the observed offset and makes future base-disturbance/compensation analysis possible. Do not merge them into one value.

## Phase 1B.2 recommendation

**Do not enter yet.** First resolve or characterize the periodic feedback discontinuity and final-zero behavior. The raw data is sufficient to continue diagnosis, but not sufficient to claim reliable stabilization or base-disturbance rejection.
