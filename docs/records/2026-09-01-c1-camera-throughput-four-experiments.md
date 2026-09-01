# C1 Camera Throughput: Four Experiments

日期：2026-09-01

设备：`/dev/v4l/by-id/usb-Wasintek_Wasintek_camera_00.00.01-video-index0`，实际采集节点 `/dev/video0`，V4L2 `MJPG 3840x2160@30`。

## 结果

| 实验 | 路径/订阅者 | 结果 |
|---|---|---|
| 1 | V4L2 MJPEG 直接读到 `/dev/null` | 900 帧 / 30.37 s，约 29.64 FPS；v4l2-ctl 内部约 29.9–30.0 FPS |
| 2 | ffmpeg MJPEG decode 到 null | 900 帧 / 30 s，30 FPS；CPU 22.51 s，RSS 约 583 MB |
| 3 | usb_cam + 1 个最轻量 rclpy subscriber | 976 帧 / 60 s，16.272 FPS；timestamp 单调，最大间隔 0.368 s |
| 4a | usb_cam + 0/1/2/3 个 subscriber | 1 个：16.792 FPS；2 个：12.056/13.021 FPS；3 个：6.259/6.123/7.426 FPS |
| 4b | 重复分档确认 | 1 个：16.299 FPS；2 个：8.705/10.304 FPS；3 个：6.516/5.553/4.371 FPS |

## 资源观察

轻量 subscriber 在 1 个订阅者时约 68–74% CPU、约 80–138 MB RSS；2 个订阅者约 56–73% CPU、约 80–177 MB RSS；3 个订阅者约 27–62% CPU、约 80–178 MB RSS。实际 `usb_cam_node_exe` PID 未能从 `ros2 run` 启动链中可靠识别，因此没有记录其 CPU/RSS，避免将包装进程数据误报为 usb_cam 数据。

## 解码告警

ffmpeg 解码到 null 过程中反复报告 `unable to decode APP fields`，并报告一次 non-monotonic DTS；尽管如此，仍以 30 FPS 输出 900 帧。该告警需要后续单独调查。

## 结论

`C1 + USB + uvcvideo` 实际吞吐正常，MJPEG 解码到 null 也能维持 30 FPS。性能下降发生在 ROS2 `usb_cam -> image_raw -> subscriber` 链路，并且随着 4K RGB subscriber 数量增加而显著恶化；当前证据支持 DDS 大消息传输/复制及 subscriber 解码处理是主要瓶颈。该结论不涉及云台、AcquireView、标定或 runtime。

完整原始日志和 JSON 位于：

`acceptance_results/throughput-four-experiments-20260831_235932/`
