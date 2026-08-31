from pathlib import Path
import sys

PKG = Path(__file__).resolve().parents[1] / 'src' / 'camera_gimbal_capability'
sys.path.insert(0, str(PKG))

from camera_gimbal_capability.policy import (  # noqa: E402
    sanitize_tag,
    stamp_to_ns,
    is_strictly_fresh,
    validate_acquire_goal,
)


def test_goal_validation_accepts_normal_request():
    errors = validate_acquire_goal(
        heading=-30.0,
        roll=0.0,
        pitch=-20.0,
        tolerance=1.5,
        timeout=8.0,
        settle_time=0.3,
        image_timeout=2.0,
        max_tolerance=10.0,
    )
    assert errors == []


def test_goal_validation_rejects_nonfinite_and_unsafe_values():
    errors = validate_acquire_goal(
        heading=float('nan'),
        roll=0.0,
        pitch=0.0,
        tolerance=99.0,
        timeout=-1.0,
        settle_time=-0.1,
        image_timeout=0.0,
        max_tolerance=10.0,
    )
    assert 'angles must be finite' in errors
    assert 'tolerance must be within (0, 10.0]' in errors
    assert 'timeout must be > 0' in errors
    assert 'settle_time must be >= 0' in errors
    assert 'image_timeout must be > 0' in errors


def test_tag_is_safe_for_filenames():
    assert sanitize_tag(' Tree 03 / left ') == 'Tree_03_left'
    assert sanitize_tag('') == 'capture'
    assert len(sanitize_tag('x' * 200)) <= 64


def test_freshness_requires_strictly_new_stamp():
    threshold = stamp_to_ns(10, 500)
    assert not is_strictly_fresh(10, 500, threshold)
    assert is_strictly_fresh(10, 501, threshold)


def test_goal_validation_rejects_mechanical_limit_violation():
    errors = validate_acquire_goal(
        heading=161.0,
        roll=0.0,
        pitch=0.0,
        tolerance=1.5,
        timeout=8.0,
        settle_time=0.3,
        image_timeout=2.0,
        max_tolerance=10.0,
    )
    assert 'angles exceed mechanical limits H[-160,160] R[-40,40] P[-90,90]' in errors


def test_optional_positive_resolution_preserves_negative_for_validation():
    from camera_gimbal_capability.policy import resolve_positive_or_default
    assert resolve_positive_or_default(0.0, 8.0) == 8.0
    assert resolve_positive_or_default(2.0, 8.0) == 2.0
    assert resolve_positive_or_default(-1.0, 8.0) == -1.0


def test_tag_preserves_unicode_alphanumeric_labels():
    assert sanitize_tag('荔枝 03 左侧') == '荔枝_03_左侧'
