# Phase 1 RAW Acceptance Freeze

日期：2026-08-31

## 冻结结论

当前交付以 ROS 2 Humble / Ubuntu 22.04 的 Phase 1 RAW Hardware Acceptance 为主流程：使用 `usb_cam`，优先 `3840x2160@30`，消费 `/cv_camera0/image_raw`，不要求标定板，不设置 camera info URL，不启动 `image_proc`，不使用 `image_rect`，保留原始广角畸变。

AcquireView 的成功条件包含云台真实稳定到位，以及一张 timestamp 严格晚于 `reached_stamp` 的新图像。驱动验收包含串口 115200 8N1、反馈和 `MovePantilt`，测试姿态序列为 `0 -> -30 -> +30 -> 0`。

## 范围边界

本阶段不耦合 runtime、Nav2 或 SLAM；上层只依赖 AcquireView capability。Phase 2 标定和 Phase 3 Inspection Integration 均延期。

## 证据规则

离线测试或模拟测试不能替代真实 C1 证据。未连接设备时，不宣称 4K30、云台运动、反馈或串口通信通过；这些项目须在验收记录中明确标为未验证，并附真实命令输出或采集记录。
