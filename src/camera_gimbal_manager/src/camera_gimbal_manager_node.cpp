#include "camera_gimbal_manager/gimbal_state_estimator.hpp"

#include <camera_gimbal_interfaces/action/acquire_view.hpp>
#include <camera_gimbal_interfaces/msg/gimbal_state.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgcodecs.hpp>
#include <pantilt_camera_serial/action/move_pantilt.hpp>
#include <pantilt_camera_serial/msg/pantilt_angle_info.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace camera_gimbal_manager
{
using AcquireView = camera_gimbal_interfaces::action::AcquireView;
using MovePantilt = pantilt_camera_serial::action::MovePantilt;
using GoalHandle = rclcpp_action::ServerGoalHandle<AcquireView>;

class CameraGimbalManager final : public rclcpp::Node
{
public:
  CameraGimbalManager()
  : Node("camera_gimbal_manager")
  {
    estimator_ = std::make_unique<GimbalStateEstimator>(
      static_cast<std::size_t>(declare_parameter<int>("filter.window_size", 5)),
      static_cast<std::size_t>(declare_parameter<int>("arrival.window_size", 10)),
      declare_parameter<double>("arrival.tolerance_deg", 1.5),
      declare_parameter<double>("arrival.stable_duration", 0.5));
    feedback_timeout_s_ = declare_parameter<double>("feedback_timeout", 1.0);
    settle_time_s_ = declare_parameter<double>("settle_time", 0.5);
    image_timeout_s_ = declare_parameter<double>("image_timeout", 3.0);
    output_directory_ = declare_parameter<std::string>("output_directory", "/tmp/camera_gimbal");
    state_pub_ = create_publisher<camera_gimbal_interfaces::msg::GimbalState>("/camera_gimbal/state", 10);
    feedback_sub_ = create_subscription<pantilt_camera_serial::msg::PantiltAngleInfo>(
      "/pantilt_camera_serial0/pantilt_angle_info", rclcpp::SensorDataQoS(),
      std::bind(&CameraGimbalManager::feedback_callback, this, std::placeholders::_1));
    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      "/cv_camera0/image_raw", rclcpp::SensorDataQoS(),
      std::bind(&CameraGimbalManager::image_callback, this, std::placeholders::_1));
    move_client_ = rclcpp_action::create_client<MovePantilt>(this, "/pantilt_camera_serial0/move_pantilt");
    action_server_ = rclcpp_action::create_server<AcquireView>(
      this, "/camera_gimbal/acquire_view",
      std::bind(&CameraGimbalManager::goal_callback, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&CameraGimbalManager::cancel_callback, this, std::placeholders::_1),
      std::bind(&CameraGimbalManager::accepted_callback, this, std::placeholders::_1));
    state_timer_ = create_wall_timer(std::chrono::milliseconds(50), std::bind(&CameraGimbalManager::publish_state, this));
  }

private:
  rclcpp_action::GoalResponse goal_callback(const rclcpp_action::GoalUUID &, std::shared_ptr<const AcquireView::Goal> goal)
  {
    if (!std::isfinite(goal->heading) || !std::isfinite(goal->pitch) || goal->timeout < 0.0) {
      return rclcpp_action::GoalResponse::REJECT;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return active_ ? rclcpp_action::GoalResponse::REJECT : rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse cancel_callback(const std::shared_ptr<GoalHandle>)
  {
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void accepted_callback(const std::shared_ptr<GoalHandle> handle)
  {
    std::thread([this, handle]() {execute(handle);}).detach();
  }

  void feedback_callback(const pantilt_camera_serial::msg::PantiltAngleInfo::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    raw_msg_ = *msg;
    estimator_->update({msg->encoder_heading, msg->encoder_roll, msg->encoder_pitch},
      {msg->heading, msg->roll, msg->pitch}, GimbalStateEstimator::Clock::now());
    last_feedback_ = GimbalStateEstimator::Clock::now();
    ++feedback_sequence_;
    have_feedback_ = true;
  }

  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    last_image_ = msg;
  }

  bool feedback_alive_locked() const
  {
    return have_feedback_ && std::chrono::duration<double>(
      GimbalStateEstimator::Clock::now() - last_feedback_).count() <= feedback_timeout_s_;
  }

  void publish_state()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    camera_gimbal_interfaces::msg::GimbalState state;
    state.header = raw_msg_.header;
    state.header.stamp = now();
    state.raw_encoder_heading = estimator_->raw().heading;
    state.raw_encoder_roll = estimator_->raw().roll;
    state.raw_encoder_pitch = estimator_->raw().pitch;
    state.filtered_encoder_heading = estimator_->filtered().heading;
    state.filtered_encoder_roll = estimator_->filtered().roll;
    state.filtered_encoder_pitch = estimator_->filtered().pitch;
    state.heading = estimator_->filtered().heading;
    state.roll = estimator_->filtered().roll;
    state.pitch = estimator_->filtered().pitch;
    state.feedback_alive = feedback_alive_locked();
    state.stable = state.feedback_alive && estimator_->stable();
    state_pub_->publish(state);
  }

  void finish(const std::shared_ptr<GoalHandle> & handle, const std::shared_ptr<AcquireView::Result> & result,
    bool success, uint16_t code, const std::string & message)
  {
    result->success = success; result->error_code = code; result->message = message;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      result->encoder_heading = estimator_->filtered().heading;
      result->encoder_pitch = estimator_->filtered().pitch;
      result->heading = estimator_->filtered().heading;
      result->pitch = estimator_->filtered().pitch;
      active_ = false;
    }
    if (success) handle->succeed(result); else handle->abort(result);
  }

  void publish_feedback(const std::shared_ptr<GoalHandle> & handle, uint8_t state, float progress)
  {
    auto feedback = std::make_shared<AcquireView::Feedback>();
    feedback->state = state; feedback->progress = progress;
    std::lock_guard<std::mutex> lock(mutex_);
    handle->publish_feedback(feedback);
  }

  void execute(const std::shared_ptr<GoalHandle> & handle)
  {
    const auto goal = handle->get_goal();
    auto result = std::make_shared<AcquireView::Result>();
    const double timeout = goal->timeout > 0.0 ? goal->timeout : 8.0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(timeout);
    uint64_t feedback_before_move;
    {std::lock_guard<std::mutex> lock(mutex_); active_ = true; feedback_before_move = feedback_sequence_;}
    if (!move_client_->wait_for_action_server(std::chrono::duration<double>(timeout))) {
      finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_UNAVAILABLE, "MovePantilt unavailable"); return;
    }
    MovePantilt::Goal move;
    move.heading = goal->heading; move.pitch = goal->pitch; move.roll = goal->roll;
    move.tolerance = goal->tolerance; move.timeout = timeout; move.stable_samples = goal->stable_samples;
    auto send = move_client_->async_send_goal(move);
    if (send.wait_for(std::chrono::duration<double>(timeout)) != std::future_status::ready) {
      finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_FAILED, "MovePantilt failed"); return;
    }
    auto low_handle = send.get();
    if (!low_handle) {finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_REJECTED, "MovePantilt rejected"); return;}
    auto low_result_future = move_client_->async_get_result(low_handle);
    if (low_result_future.wait_for(std::chrono::duration<double>(timeout)) != std::future_status::ready) {
      finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_FAILED, "MovePantilt timeout"); return;
    }
    const auto low_result = low_result_future.get().result;
    if (!low_result->success) {finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_FAILED, low_result->message); return;}
    const auto reached_stamp = now();
    result->reached_stamp = reached_stamp;
    publish_feedback(handle, AcquireView::Feedback::STATE_SETTLING, 0.45F);
    const double settling = goal->settle_time > 0.0 ? goal->settle_time : settle_time_s_;
    std::this_thread::sleep_for(std::chrono::duration<double>(settling));
    bool stable = false;
    {std::lock_guard<std::mutex> lock(mutex_); stable = feedback_sequence_ > feedback_before_move &&
      feedback_alive_locked() && estimator_->stable() &&
      std::abs(estimator_->filtered().heading - goal->heading) < std::max(0.01, goal->tolerance) &&
      std::abs(estimator_->filtered().pitch - goal->pitch) < std::max(0.01, goal->tolerance);}
    if (!stable) {finish(handle, result, false, AcquireView::Result::ERROR_GIMBAL_FAILED, "feedback not stable"); return;}
    publish_feedback(handle, AcquireView::Feedback::STATE_WAITING_IMAGE, 0.7F);
    sensor_msgs::msg::Image::SharedPtr image;
    const auto image_deadline = std::min(deadline, std::chrono::steady_clock::now() + std::chrono::duration<double>(goal->image_timeout > 0.0 ? goal->image_timeout : image_timeout_s_));
    while (std::chrono::steady_clock::now() < image_deadline && rclcpp::ok()) {
      {std::lock_guard<std::mutex> lock(mutex_); image = last_image_;}
      if (image && rclcpp::Time(image->header.stamp) > rclcpp::Time(reached_stamp)) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!image || rclcpp::Time(image->header.stamp) <= rclcpp::Time(reached_stamp)) {
      finish(handle, result, false, AcquireView::Result::ERROR_IMAGE_TIMEOUT, "no fresh image"); return;
    }
    try {
      std::filesystem::create_directories(output_directory_);
      const std::string name = goal->capture_name.empty() ? (goal->tag.empty() ? "capture" : goal->tag) : goal->capture_name;
      const std::string path = output_directory_ + "/" + name + ".png";
      if (!cv::imwrite(path, cv_bridge::toCvCopy(image, "bgr8")->image)) throw std::runtime_error("image write failed");
      result->image_path = path; result->image_stamp = image->header.stamp; result->capture_stamp = image->header.stamp;
    } catch (const std::exception & e) {finish(handle, result, false, AcquireView::Result::ERROR_IMAGE_SAVE_FAILED, e.what()); return;}
    finish(handle, result, true, AcquireView::Result::ERROR_OK, "capture complete");
  }

  std::unique_ptr<GimbalStateEstimator> estimator_;
  pantilt_camera_serial::msg::PantiltAngleInfo raw_msg_;
  sensor_msgs::msg::Image::SharedPtr last_image_;
  rclcpp::Publisher<camera_gimbal_interfaces::msg::GimbalState>::SharedPtr state_pub_;
  rclcpp::Subscription<pantilt_camera_serial::msg::PantiltAngleInfo>::SharedPtr feedback_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp_action::Client<MovePantilt>::SharedPtr move_client_;
  rclcpp_action::Server<AcquireView>::SharedPtr action_server_;
  rclcpp::TimerBase::SharedPtr state_timer_;
  mutable std::mutex mutex_;
  GimbalStateEstimator::TimePoint last_feedback_{};
  double feedback_timeout_s_{1.0}, settle_time_s_{0.5}, image_timeout_s_{3.0};
  std::string output_directory_;
  uint64_t feedback_sequence_{0};
  bool have_feedback_{false}, active_{false};
};
}  // namespace camera_gimbal_manager

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<camera_gimbal_manager::CameraGimbalManager>());
  rclcpp::shutdown();
  return 0;
}
