# Autolabor C1 Camera-Gimbal ROS2 Capability

这是 Autolabor C1 相机云台的 **ROS2 Humble 原子能力交付包**。目标不是承担导航或巡检任务编排，而是向任意任务控制系统提供一个稳定的：

> `姿态目标 -> 云台真正稳定到位 -> 获取稳定后的新图像 -> 结构化返回`

能力。

## 包结构

```text
src/
├── camera_gimbal_interfaces/      # 对控制系统公开的 Action/Health
├── camera_gimbal_capability/      # AcquireView 业务能力层
├── pantilt_camera_serial/         # 串口协议 + MovePantilt 低层可靠运动
├── autolabor_c1_bringup/          # 一键启动、实机/模拟验收工具
└── rviz_pantilt_plugin/           # RViz2 人工调试面板
```

### 与导航解耦

本工作空间**不负责**：Nav2、地图、巡检点、任务顺序、机器人 TF 位姿记录、回原点、巡检报告。

控制系统只依赖 `camera_gimbal_interfaces`，不需要依赖云台串口实现。

接口冻结见 [CAPABILITY_INTERFACE.md](CAPABILITY_INTERFACE.md)。

## 环境

- Ubuntu 22.04
- ROS2 Humble
- `usb_cam`
- Boost.Asio
- OpenCV / `cv_bridge`
- RViz2（仅调试需要）

安装依赖：

```bash
git clone https://github.com/Aldoubt/Autolabor-C1-ROS2.git
cd Autolabor-C1-ROS2
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
```

构建：

```bash
colcon build --symlink-install --event-handlers console_direct+
source install/setup.bash
```

## 启动

当前 Phase 1 默认使用 **RAW / 无内参 / 无畸变矫正** 模式。先验证相机、云台和新图采集，不让缺少标定板阻塞硬件验收。

```bash
ros2 launch autolabor_c1_bringup autolabor_c1.launch.py \
  gui:=false \
  device_path:=/dev/v4l/by-id/<C1-video-device> \
  port_name:=/dev/serial/by-id/<C1-control-device> \
  image_width:=3840 image_height:=2160 fps:=30.0 \
  camera_info_url:="" rectify:=false \
  capability_image_topic:=/cv_camera0/image_raw
```

拿到实体标定板后再进入 Phase 2，使用 ROS2 `camera_calibration` 生成本机内参，并切换到 `image_proc -> image_rect`。详见 [CAMERA_CALIBRATION_ROS2.md](CAMERA_CALIBRATION_ROS2.md)。

主要业务接口：

| 接口 | 类型 | 定位 |
|---|---|---|
| `/camera_gimbal/acquire_view` | `camera_gimbal_interfaces/action/AcquireView` | **控制系统首选公开接口** |
| `/camera_gimbal/health` | `camera_gimbal_interfaces/msg/CapabilityHealth` | READY/BUSY/DEGRADED/ERROR |
| `/pantilt_camera_serial0/move_pantilt` | `pantilt_camera_serial/action/MovePantilt` | 驱动级运动验收/调试 |
| `/pantilt_camera_serial0/pantilt_status` | `pantilt_camera_serial/msg/PantiltStatus` | 驱动诊断 |
| `/cv_camera0/image_raw` | `sensor_msgs/msg/Image` | 标准相机流 |

### AcquireView 示例

```bash
ros2 action send_goal /camera_gimbal/acquire_view \
  camera_gimbal_interfaces/action/AcquireView \
  "{heading: -30.0, roll: 0.0, pitch: 0.0, tolerance: 0.0, timeout: 0.0, stable_samples: 0, settle_time: 0.0, image_timeout: 0.0, save_image: true, tag: 'P01_left'}" \
  --feedback
```

其中 `0` 表示使用 capability 节点的默认时间/容差参数。

`AcquireView success=true` 必须同时满足云台稳定到位和获取稳定后的严格新图像；机器验收还会再次验证 `image_stamp > reached_stamp`。

## 底层状态可信度

`MovePantilt` 不把“串口 write 成功”当作运动成功。真正 `REACHED` 需要：

1. 持续收到新编码器反馈；
2. Heading / Roll / Pitch 全部进入容差；
3. 连续 `stable_samples` 个不同反馈帧都满足容差；
4. 期间没有串口断开、反馈超时、动作超时或取消。

默认：反馈查询 10 Hz、容差 1.5°、连续稳定 3 帧。

底层兼容 Service 只表示“参数校验 + 串口写入”，使用运动类 Service 后状态标记 `UNVERIFIED`，不能被任务层当作成功。

### 公共错误码

控制系统按 `error_code` 分支，不解析 `message` 字符串：

| code | 常量 | 含义 |
|---:|---|---|
| 0 | `ERROR_OK` | 成功 |
| 100 | `ERROR_INVALID_GOAL` | 参数/角度非法 |
| 101 | `ERROR_BUSY` | capability 正在执行另一目标 |
| 200 | `ERROR_GIMBAL_UNAVAILABLE` | 串口/反馈/低层 Action 不可用 |
| 201 | `ERROR_GIMBAL_REJECTED` | 低层 goal 被拒绝 |
| 202 | `ERROR_GIMBAL_FAILED` | 低层运动执行失败或结果超时 |
| 300 | `ERROR_CAMERA_UNAVAILABLE` | 执行前相机流不可用 |
| 301 | `ERROR_IMAGE_TIMEOUT` | 到位后没有严格的新图像 |
| 302 | `ERROR_IMAGE_SAVE_FAILED` | 图像保存失败 |
| 400 | `ERROR_CANCELED` | 上层取消 |
| 900 | `ERROR_INTERNAL` | capability 内部异常 |

## 验收

### 1. Codex 离线门禁

```bash
./tools/run_acceptance.sh offline
```

执行源码 pytest、纯 C++ 协议/到位判定测试、Python 语法检查、`colcon build` 和 `colcon test`。

### 2. Codex 无实机集成门禁

```bash
./tools/run_acceptance.sh simulated
```

自动验证：

- 正常 `AcquireView`：0° / -30° / +30° / 回中；
- 非法 999° 必须返回 `ERROR_INVALID_GOAL=100`；
- 模拟云台不可用必须返回 `ERROR_GIMBAL_UNAVAILABLE=200`；
- 模拟云台运动失败必须返回 `ERROR_GIMBAL_FAILED=202`；
- 启动前相机断流必须返回 `ERROR_CAMERA_UNAVAILABLE=300`；
- 云台运动后相机断流必须返回 `ERROR_IMAGE_TIMEOUT=301`，不得复用旧帧；
- 取消公共 Action 必须返回 `ERROR_CANCELED=400`，并确认低层运动已退出；
- 成功图像必须满足 `image_stamp > reached_stamp`。

### 3. Codex + 人工实机门禁

```bash
CAMERA_DEVICE=/dev/v4l/by-id/<C1-video-device> \
GIMBAL_PORT=/dev/serial/by-id/<C1-control-device> \
./tools/run_acceptance.sh hardware
```

脚本输出 `MACHINE PASS` 后，再按 [HUMAN_ACCEPTANCE_CHECKLIST.md](HUMAN_ACCEPTANCE_CHECKLIST.md) 做人工物理确认。

给 Codex 的固定执行约束见 [CODEX_ACCEPTANCE_PROMPT.md](CODEX_ACCEPTANCE_PROMPT.md)。

## 单独调试

驱动健康：

```bash
ros2 run autolabor_c1_bringup check_status.py
```

低层 MovePantilt + 相机旧验收入口仍保留：

```bash
ros2 run autolabor_c1_bringup test_gimbal.py
```

公开 capability 验收：

```bash
ros2 run autolabor_c1_bringup test_capability.py
```

## 当前明确限制

- 串口物理断开后当前不会自动重连，需重启驱动节点。
- 原 ROS1 源码的发送/反馈字节序约定互不相同；当前保留旧实现的真实行为，没有在缺少厂商协议/抓包证据时擅自改变。
- 取消/运动超时时会在反馈仍新鲜时 best-effort 下发当前编码器姿态保持命令，但这**不是安全级急停**。
- 3840×2160@30 Hz 需要在目标相机、USB 带宽和 `usb_cam` 上实测确认。

## 分阶段验收与巡检接入

当前冻结为：

```text
Phase 1  RAW Hardware Acceptance
  - 3840x2160@30 优先
  - image_raw
  - 无内参门禁
  - 不启动 image_proc
  - 云台运动 + AcquireView 新图验收

Phase 2  Camera Calibration
  - ROS2 camera_calibration
  - 本机 c1_3840x2160.yaml
  - CameraInfo + image_proc
  - image_rect 验收

Phase 3  Inspection Integration
  - 巡检系统只调用 AcquireView
  - 根据 Phase 2 验收结果把 capability_image_topic 切到 image_rect
```

相机单独测试：

```bash
CAMERA_DEVICE=/dev/v4l/by-id/<C1-video-device> \
./tools/run_camera_offline_test.sh
```

当前 Phase 1 的 `camera_metrics.json` 只把 raw 分辨率、raw FPS 和时间戳单调性作为硬门禁；CameraInfo 与 rectification 不参与 PASS。详见 [CAMERA_OFFLINE_ACCEPTANCE.md](CAMERA_OFFLINE_ACCEPTANCE.md)。
