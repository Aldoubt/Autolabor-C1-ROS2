from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def test_driver_is_ros2_ament_and_generates_action_interfaces():
    cmake = read("src/pantilt_camera_serial/CMakeLists.txt")
    pkg = read("src/pantilt_camera_serial/package.xml")
    assert "ament_cmake" in cmake
    assert "rosidl_generate_interfaces" in cmake
    assert "action/MovePantilt.action" in cmake
    assert "catkin" not in cmake
    assert "<buildtool_depend>ament_cmake</buildtool_depend>" in pkg
    assert "catkin" not in pkg


def test_feedback_has_timestamp_and_status_interface_exists():
    angle = read("src/pantilt_camera_serial/msg/PantiltAngleInfo.msg")
    assert "std_msgs/Header header" in angle
    status = ROOT / "src/pantilt_camera_serial/msg/PantiltStatus.msg"
    action = ROOT / "src/pantilt_camera_serial/action/MovePantilt.action"
    assert status.exists()
    assert action.exists()
    assert "stable_samples" in action.read_text(encoding="utf-8")
    assert "result_code" in action.read_text(encoding="utf-8")


def test_driver_sources_use_ros2_headers_only():
    source_files = [
        ROOT / "src/pantilt_camera_serial/src/pantilt_serial_control.cpp",
        ROOT / "src/pantilt_camera_serial/include/pantilt_camera_serial/pantilt_serial_control.hpp",
    ]
    for p in source_files:
        assert p.exists(), p
        text = p.read_text(encoding="utf-8")
        assert "rclcpp" in text
        assert "ros/ros.h" not in text
        assert "ROS_INFO" not in text


def test_bringup_is_ros2_launch_and_scripts_are_executable():
    assert (ROOT / "src/autolabor_c1_bringup/CMakeLists.txt").exists()
    launch = ROOT / "src/autolabor_c1_bringup/launch/autolabor_c1.launch.py"
    assert launch.exists()
    launch_text = launch.read_text(encoding="utf-8")
    assert "usb_cam" in launch_text
    assert "pantilt_camera_serial" in launch_text
    for script in ["check_status.py", "test_gimbal.py"]:
        p = ROOT / "src/autolabor_c1_bringup/scripts" / script
        text = p.read_text(encoding="utf-8")
        assert "rclpy" in text
        assert os.access(p, os.X_OK)


def test_rviz_plugin_is_ported_to_rviz2():
    cmake = read("src/rviz_pantilt_plugin/CMakeLists.txt")
    header = read("src/rviz_pantilt_plugin/include/rviz_pantilt_plugin/pantilt_plugin.hpp")
    xml = read("src/rviz_pantilt_plugin/plugin_description.xml")
    assert "rviz_common" in cmake
    assert "ament_cmake" in cmake
    assert "rviz_common::Panel" in header
    assert "base_class_type=\"rviz_common::Panel\"" in xml
    assert "catkin" not in cmake


def test_no_ros1_build_system_or_runtime_in_active_sources():
    for p in SRC.rglob("*"):
        if not p.is_file() or p.suffix in {".png", ".rviz"}:
            continue
        if p.name.endswith(".md"):
            continue
        text = p.read_text(encoding="utf-8", errors="ignore")
        assert "<buildtool_depend>catkin</buildtool_depend>" not in text, p
        assert "#include <ros/ros.h>" not in text, p
        assert "import rospy" not in text, p


def test_status_exposes_stabilizing_state():
    msg = (ROOT / 'src/pantilt_camera_serial/msg/PantiltStatus.msg').read_text()
    assert 'STATE_STABILIZING' in msg


def test_sensor_subscriptions_use_sensor_qos_and_launch_types_are_explicit():
    bringup = (ROOT / 'src/autolabor_c1_bringup/launch/autolabor_c1.launch.py').read_text()
    acceptance = (ROOT / 'src/autolabor_c1_bringup/scripts/test_gimbal.py').read_text()
    health = (ROOT / 'src/autolabor_c1_bringup/scripts/check_status.py').read_text()
    assert 'ParameterValue' in bringup
    assert 'qos_profile_sensor_data' in acceptance
    assert 'qos_profile_sensor_data' in health


def test_public_capability_interfaces_are_separate_from_driver():
    interfaces = ROOT / 'src/camera_gimbal_interfaces'
    assert interfaces.exists()
    action = (interfaces / 'action/AcquireView.action').read_text(encoding='utf-8')
    health = (interfaces / 'msg/CapabilityHealth.msg').read_text(encoding='utf-8')
    assert 'builtin_interfaces/Time reached_stamp' in action
    assert 'builtin_interfaces/Time image_stamp' in action
    assert 'uint16 ERROR_IMAGE_TIMEOUT=301' in action
    assert 'STATE_READY' in health
    assert 'camera_alive' in health


def test_capability_package_and_bringup_are_wired():
    pkg = ROOT / 'src/camera_gimbal_capability'
    assert (pkg / 'package.xml').exists()
    assert (pkg / 'setup.py').exists()
    node = (pkg / 'camera_gimbal_capability/capability_node.py').read_text(encoding='utf-8')
    assert 'AcquireView' in node
    assert 'MovePantilt' in node
    assert 'qos_profile_sensor_data' in node
    launch = read('src/autolabor_c1_bringup/launch/autolabor_c1.launch.py')
    assert "package='camera_gimbal_capability'" in launch


def test_acceptance_tooling_supports_machine_and_human_gates():
    assert (ROOT / 'tools/run_acceptance.sh').exists()
    assert (ROOT / 'tools/validate_acceptance.py').exists()
    assert (ROOT / 'CODEX_ACCEPTANCE_PROMPT.md').exists()
    assert (ROOT / 'HUMAN_ACCEPTANCE_CHECKLIST.md').exists()
    assert (ROOT / 'src/autolabor_c1_bringup/scripts/test_capability.py').exists()
    assert (ROOT / 'src/autolabor_c1_bringup/scripts/fake_gimbal_camera.py').exists()
    fake_launch = ROOT / 'src/autolabor_c1_bringup/launch/autolabor_c1_fake.launch.py'
    assert fake_launch.exists()


def test_cancel_and_motion_timeout_attempt_best_effort_hold():
    header = read('src/pantilt_camera_serial/include/pantilt_camera_serial/pantilt_serial_control.hpp')
    source = read('src/pantilt_camera_serial/src/pantilt_serial_control.cpp')
    assert 'best_effort_hold_current' in header
    assert source.count('best_effort_hold_current()') >= 2
    assert 'hold current position failed' in source


def test_public_cancel_waits_for_low_level_cancel_completion():
    source = read('src/camera_gimbal_capability/camera_gimbal_capability/capability_node.py')
    assert 'def _cancel_move_and_wait' in source
    assert source.count('_cancel_move_and_wait(move_handle, move_result_future)') >= 1


def test_simulated_fault_gate_exists():
    script = ROOT / 'src/autolabor_c1_bringup/scripts/test_expected_error.py'
    assert script.exists()
    text = script.read_text(encoding='utf-8')
    assert 'expected_error' in text
    runner = read('tools/run_acceptance.sh')
    assert 'publish_images:=false' in runner
    assert 'fail_motion:=true' in runner


def test_cancel_during_low_level_goal_handshake_cannot_leave_orphan_motion():
    source = read('src/camera_gimbal_capability/camera_gimbal_capability/capability_node.py')
    assert 'def _wait_future_ignore_cancel' in source
    assert '_wait_future_ignore_cancel(send_future' in source
    assert "'request canceled after gimbal goal handshake'" in source


def test_simulated_cancel_gate_exists():
    script = ROOT / 'src/autolabor_c1_bringup/scripts/test_capability_cancel.py'
    assert script.exists()
    text = script.read_text(encoding='utf-8')
    assert 'ERROR_CANCELED' in text
    assert 'target_active' in text
    runner = read('tools/run_acceptance.sh')
    assert 'simulated_cancel' in runner


def test_bringup_exposes_capability_capture_output_root():
    real_launch = read('src/autolabor_c1_bringup/launch/autolabor_c1.launch.py')
    fake_launch = read('src/autolabor_c1_bringup/launch/autolabor_c1_fake.launch.py')
    assert "DeclareLaunchArgument('capture_output_root'" in real_launch
    assert "'output_root': LaunchConfiguration('capture_output_root')" in real_launch
    assert "DeclareLaunchArgument('capture_output_root'" in fake_launch
    assert "'output_root': LaunchConfiguration('capture_output_root')" in fake_launch


def test_simulated_gimbal_unavailable_gate_exists():
    fake = read('src/autolabor_c1_bringup/scripts/fake_gimbal_camera.py')
    runner = read('tools/run_acceptance.sh')
    assert "declare_parameter('serial_connected', True)" in fake
    assert 'serial_connected:=false' in runner
    assert '-p expected_error:=200' in runner


def test_simulated_post_motion_image_timeout_gate_exists():
    fake = read('src/autolabor_c1_bringup/scripts/fake_gimbal_camera.py')
    runner = read('tools/run_acceptance.sh')
    assert "declare_parameter('drop_images_on_motion', False)" in fake
    assert 'drop_images_on_motion:=true' in runner
    assert '-p expected_error:=301' in runner


def test_acceptance_runner_uses_strict_cpp_warnings_and_resets_fake_pid():
    runner = read('tools/run_acceptance.sh')
    assert runner.count('-Werror') >= 2
    assert 'LAUNCH_PID=""' in runner


def test_expected_error_probe_waits_for_gimbal_unavailable_precondition():
    probe = read('src/autolabor_c1_bringup/scripts/test_expected_error.py')
    assert 'ERROR_GIMBAL_UNAVAILABLE' in probe
    assert 'not self._health.gimbal_serial_connected' in probe


def test_docs_list_all_negative_simulated_gates():
    readme = read('README.md')
    codex = read('CODEX_ACCEPTANCE_PROMPT.md')
    for token in [
        'ERROR_GIMBAL_UNAVAILABLE=200',
        'ERROR_GIMBAL_FAILED=202',
        'ERROR_CAMERA_UNAVAILABLE=300',
        'ERROR_IMAGE_TIMEOUT=301',
        'ERROR_CANCELED=400',
    ]:
        assert token in readme
        assert token in codex


def test_cancel_cannot_be_overwritten_by_success_after_image_capture_or_save():
    source = read('src/camera_gimbal_capability/camera_gimbal_capability/capability_node.py')
    assert 'request canceled after image capture' in source
    assert 'request canceled after image save' in source
