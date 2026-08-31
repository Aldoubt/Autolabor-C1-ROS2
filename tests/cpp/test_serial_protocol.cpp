#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include "pantilt_camera_serial/serial_protocol.hpp"

using autolabor_driver::PantiltUtils;

int main()
{
  // Characterization: preserve the original driver's actual outgoing byte order.
  const auto bytes = PantiltUtils::int16_to_protocol_bytes(static_cast<std::int16_t>(0x1234));
  assert(bytes.size() == 2U);
  assert(bytes[0] == 0x12U);
  assert(bytes[1] == 0x34U);

  const auto motion = PantiltUtils::prepare_motion_data(30.0, 0.0, -10.0);
  assert(motion.size() == 7U);
  assert(motion[0] == 0x01U && motion[1] == 0x2CU);  // +300 -> 0x012C
  assert(motion[4] == 0xFFU && motion[5] == 0x9CU);  // -100 -> 0xFF9C
  assert(PantiltUtils::prepare_motion_data(161.0, 0.0, 0.0).empty());
  assert(PantiltUtils::prepare_motion_data(NAN, 0.0, 0.0).empty());

  // New safety contract: speed commands are finite and bounded to +/-2.0.
  assert(PantiltUtils::prepare_speed_data(2.0, -2.0).size() == 4U);
  assert(PantiltUtils::prepare_speed_data(2.01, 0.0).empty());
  assert(PantiltUtils::prepare_speed_data(0.0, -2.01).empty());
  assert(PantiltUtils::prepare_speed_data(NAN, 0.0).empty());

  const auto query = PantiltUtils::build_command("GetPantiltPose", {});
  assert(query.size() == query[1]);
  assert(query.front() == 0xAAU);
  assert(query.back() == PantiltUtils::crc8_calculate(
    std::vector<std::uint8_t>(query.begin(), query.end() - 1)));
  assert(PantiltUtils::build_command("DefinitelyUnknown", {}).empty());
  assert(PantiltUtils::is_mutating_command("SetLockMode"));
  assert(PantiltUtils::is_mutating_command("SetHeadingFollow"));
  assert(PantiltUtils::is_mutating_command("BackToCenter"));
  assert(!PantiltUtils::is_mutating_command("GetPantiltPose"));

  // Characterization: incoming angle feedback remains little-endian as in ROS1.
  std::vector<std::uint8_t> frame(18U, 0U);
  frame[4] = 0xD2; frame[5] = 0x04;  // +1234 / 100 = 12.34 deg
  frame[10] = 0xC7; frame[11] = 0xCF;  // -12345 / 100 = -123.45 deg
  const auto parsed = PantiltUtils::parse_angles(frame);
  assert(parsed.has_value());
  assert(std::abs(parsed->ground.heading - 12.34) < 1e-9);
  assert(std::abs(parsed->encoder.heading + 123.45) < 1e-9);
  return 0;
}
