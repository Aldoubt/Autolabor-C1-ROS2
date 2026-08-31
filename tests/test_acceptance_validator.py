import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / 'tools' / 'validate_acceptance.py'


def load_module():
    spec = importlib.util.spec_from_file_location('validate_acceptance', MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def good_summary(image_path):
    valid_case = {
        'success': True,
        'error_code': 0,
        'pass': True,
        'reached_stamp_ns': 100,
        'image_stamp_ns': 101,
        'image_path': str(image_path),
        'image_exists': True,
        'fresh_after_reached': True,
        'max_error': 0.2,
    }
    return {
        'schema_version': 1,
        'kind': 'camera_gimbal_capability_acceptance',
        'passed': True,
        'tests': [
            {
                'name': 'invalid_goal',
                'success': False,
                'error_code': 100,
                'pass': True,
                'reached_stamp_ns': 0,
                'image_stamp_ns': 0,
                'image_path': '',
            },
            dict(valid_case, name='center'),
            dict(valid_case, name='left'),
            dict(valid_case, name='right'),
            dict(valid_case, name='return_center'),
        ],
    }


def test_validator_accepts_strict_good_summary(tmp_path):
    image = tmp_path / 'center.png'
    image.write_bytes(b'png')
    module = load_module()
    errors = module.validate_summary(good_summary(image), require_images=True)
    assert errors == []


def test_validator_rejects_fake_success_and_stale_image(tmp_path):
    image = tmp_path / 'center.png'
    image.write_bytes(b'png')
    data = good_summary(image)
    data['tests'][1]['image_stamp_ns'] = 99
    data['tests'][1]['fresh_after_reached'] = False
    module = load_module()
    errors = module.validate_summary(data, require_images=True)
    assert any('not newer than reached' in item for item in errors)


def test_validator_rejects_top_level_pass_with_failed_case(tmp_path):
    image = tmp_path / 'center.png'
    image.write_bytes(b'png')
    data = good_summary(image)
    data['tests'][1]['pass'] = False
    module = load_module()
    errors = module.validate_summary(data, require_images=True)
    assert any('case center is not PASS' in item for item in errors)
