#include <cassert>
#include <cmath>
#include <pantilt_camera_serial/arrival_judge.hpp>

using autolabor_driver::ArrivalJudge;
using autolabor_driver::Angles;

int main() {
  ArrivalJudge judge(Angles{30.0, 0.0, -10.0}, 1.0, 3);
  assert(!judge.observe(Angles{28.0, 0.0, -10.0}));
  assert(judge.stable_count() == 0);
  assert(!judge.observe(Angles{29.4, 0.4, -10.2}));
  assert(judge.stable_count() == 1);
  assert(!judge.observe(Angles{30.4, -0.2, -9.7}));
  assert(judge.stable_count() == 2);
  assert(judge.observe(Angles{30.2, 0.1, -10.1}));
  assert(judge.reached());
  assert(judge.max_error(Angles{31.5, 0.0, -10.0}) == 1.5);

  judge.reset(Angles{-30.0, 0.0, 0.0}, 0.5, 2);
  assert(!judge.observe(Angles{-30.3, 0.1, 0.0}));
  assert(judge.stable_count() == 1);
  assert(!judge.observe(Angles{-29.0, 0.0, 0.0}));
  assert(judge.stable_count() == 0);
  assert(!judge.observe(Angles{-30.2, 0.0, 0.0}));
  assert(judge.observe(Angles{-30.1, 0.0, 0.0}));
  return 0;
}
