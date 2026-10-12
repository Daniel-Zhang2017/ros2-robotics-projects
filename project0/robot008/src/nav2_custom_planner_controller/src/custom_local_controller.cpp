#include "custom_local_controller/custom_local_controller.hpp"

#include <cmath>
#include <algorithm>
#include <limits>

#include "pluginlib/class_list_macros.hpp"
#include "nav2_util/node_utils.hpp"
#include "tf2/utils.h"

namespace nav2_custom_planner_controller
{

void CustomLocalController::configure(
  const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
  std::string name,
  std::shared_ptr<tf2_ros::Buffer> tf,
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros)
{
  node_ = parent;
  plugin_name_ = name;
  tf_ = tf;
  costmap_ros_ = costmap_ros;
  costmap_ = costmap_ros_->getCostmap();
  last_control_ = Eigen::Vector2d::Zero();

  auto node = node_.lock();
  if (!node) throw std::runtime_error{"Failed to lock node"};

  // Declare & load MPC parameters
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".prediction_horizon", rclcpp::ParameterValue(15));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".dt", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".max_iterations", rclcpp::ParameterValue(100));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".convergence_tol", rclcpp::ParameterValue(1e-4));

  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".q_x", rclcpp::ParameterValue(10.0));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".q_y", rclcpp::ParameterValue(10.0));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".q_theta", rclcpp::ParameterValue(2.0));

  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".r_v", rclcpp::ParameterValue(0.5));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".r_omega", rclcpp::ParameterValue(0.3));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".r_delta_v", rclcpp::ParameterValue(1.0));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".r_delta_omega", rclcpp::ParameterValue(0.8));

  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".w_obstacle", rclcpp::ParameterValue(8.0));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".safe_distance", rclcpp::ParameterValue(0.4));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".critical_distance", rclcpp::ParameterValue(0.25));

  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".max_vel_x", rclcpp::ParameterValue(0.6));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".max_vel_theta", rclcpp::ParameterValue(1.2));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".max_acc_x", rclcpp::ParameterValue(1.0));
  nav2_util::declare_parameter_if_not_declared(
    node, plugin_name_ + ".max_acc_theta", rclcpp::ParameterValue(2.0));

  // Load all parameters into struct
  node->get_parameter(plugin_name_ + ".prediction_horizon", params_.prediction_horizon);
  node->get_parameter(plugin_name_ + ".dt", params_.dt);
  node->get_parameter(plugin_name_ + ".max_iterations", params_.max_iterations);
  node->get_parameter(plugin_name_ + ".convergence_tol", params_.convergence_tol);
  node->get_parameter(plugin_name_ + ".q_x", params_.q_x);
  node->get_parameter(plugin_name_ + ".q_y", params_.q_y);
  node->get_parameter(plugin_name_ + ".q_theta", params_.q_theta);
  node->get_parameter(plugin_name_ + ".r_v", params_.r_v);
  node->get_parameter(plugin_name_ + ".r_omega", params_.r_omega);
  node->get_parameter(plugin_name_ + ".r_delta_v", params_.r_delta_v);
  node->get_parameter(plugin_name_ + ".r_delta_omega", params_.r_delta_omega);
  node->get_parameter(plugin_name_ + ".w_obstacle", params_.w_obstacle);
  node->get_parameter(plugin_name_ + ".safe_distance", params_.safe_distance);
  node->get_parameter(plugin_name_ + ".critical_distance", params_.critical_distance);
  node->get_parameter(plugin_name_ + ".max_vel_x", params_.max_vel_x);
  node->get_parameter(plugin_name_ + ".max_vel_theta", params_.max_vel_theta);
  node->get_parameter(plugin_name_ + ".max_acc_x", params_.max_acc_x);
  node->get_parameter(plugin_name_ + ".max_acc_theta", params_.max_acc_theta);

  marker_pub_ = node->create_publisher<visualization_msgs::msg::MarkerArray>(
    "custom_mpc/debug_markers", rclcpp::QoS(1));

  RCLCPP_INFO(logger_, "Custom MPC Controller configured (N=%d, dt=%.2fs)",
    params_.prediction_horizon, params_.dt);
}

void CustomLocalController::cleanup() { marker_pub_.reset(); }
void CustomLocalController::activate() {}
void CustomLocalController::deactivate() {}

void CustomLocalController::setPlan(const nav_msgs::msg::Path & path)
{
  global_plan_ = path;
}

// ------------------------------
// Reference trajectory generation
// ------------------------------
std::vector<Eigen::Vector3d> CustomLocalController::generateReferenceTrajectory(
  const geometry_msgs::msg::PoseStamped & current_pose)
{
  std::vector<Eigen::Vector3d> ref_traj;
  ref_traj.reserve(params_.prediction_horizon);

  if (global_plan_.poses.empty()) return ref_traj;

  // Find closest point on global plan
  double min_dist = std::numeric_limits<double>::max();
  size_t closest_idx = 0;
  double rx = current_pose.pose.position.x;
  double ry = current_pose.pose.position.y;

  for (size_t i = 0; i < global_plan_.poses.size(); ++i) {
    double dx = global_plan_.poses[i].pose.position.x - rx;
    double dy = global_plan_.poses[i].pose.position.y - ry;
    double d = dx*dx + dy*dy;
    if (d < min_dist) { min_dist = d; closest_idx = i; }
  }

  // Sample N points ahead along global plan
  double s = 0.0;
  ref_traj.emplace_back(
    global_plan_.poses[closest_idx].pose.position.x,
    global_plan_.poses[closest_idx].pose.position.y,
    tf2::getYaw(global_plan_.poses[closest_idx].pose.orientation));

  for (int k = 1; k < params_.prediction_horizon; ++k) {
    s += params_.dt * params_.max_vel_x * 0.5; // approximate arc length
    size_t idx = closest_idx;
    double acc = 0.0;

    while (idx < global_plan_.poses.size() - 1 && acc < s) {
      double dx = global_plan_.poses[idx+1].pose.position.x - global_plan_.poses[idx].pose.position.x;
      double dy = global_plan_.poses[idx+1].pose.position.y - global_plan_.poses[idx].pose.position.y;
      acc += std::hypot(dx, dy);
      idx++;
    }

    if (idx >= global_plan_.poses.size()) idx = global_plan_.poses.size() - 1;
    ref_traj.emplace_back(
      global_plan_.poses[idx].pose.position.x,
      global_plan_.poses[idx].pose.position.y,
      tf2::getYaw(global_plan_.poses[idx].pose.orientation));
  }

  return ref_traj;
}

// ------------------------------
// Differential drive kinematics
// ------------------------------
Eigen::Vector3d CustomLocalController::propagateState(
  const Eigen::Vector3d & state,
  const Eigen::Vector2d & u,
  double dt)
{
  double x = state(0);
  double y = state(1);
  double theta = state(2);
  double v = u(0);
  double w = u(1);

  // Euler integration (use Runge-Kutta for higher accuracy)
  Eigen::Vector3d next_state;
  next_state(0) = x + v * std::cos(theta) * dt;
  next_state(1) = y + v * std::sin(theta) * dt;
  next_state(2) = theta + w * dt;

  // Normalize angle
  while (next_state(2) > M_PI) next_state(2) -= 2.0 * M_PI;
  while (next_state(2) < -M_PI) next_state(2) += 2.0 * M_PI;

  return next_state;
}

// ------------------------------
// Obstacle cost from costmap
// ------------------------------
double CustomLocalController::computeObstacleCost(double wx, double wy)
{
  unsigned int mx, my;
  if (!costmap_->worldToMap(wx, wy, mx, my)) return 0.0;

  unsigned char cost = costmap_->getCost(mx, my);
  if (cost == nav2_costmap_2d::FREE_SPACE) return 0.0;

  // Approximate distance from cost value (simplified; use distance transform for accuracy)
  double resolution = costmap_->getResolution();
  double dist = (253.0 - cost) / 253.0 * params_.safe_distance * 3.0;
  dist = std::max(dist, 0.01);

  // Quadratic penalty when inside safe distance
  if (dist < params_.safe_distance) {
    double diff = params_.safe_distance - dist;
    return params_.w_obstacle * diff * diff;
  }
  return 0.0;
}

// ------------------------------
// MPC solver (gradient descent)
// ------------------------------
Eigen::Vector2d CustomLocalController::solveMpc(
  const Eigen::Vector3d & current_state,
  const std::vector<Eigen::Vector3d> & ref_traj,
  const Eigen::Vector2d & last_control)
{
  int N = params_.prediction_horizon;
  if ((int)ref_traj.size() < N) return Eigen::Vector2d::Zero();

  // Initialize control sequence (warm start with previous solution)
  std::vector<Eigen::Vector2d> U(N, last_control);

  double learning_rate = 0.001;
  double prev_cost = 1e9;

  for (int iter = 0; iter < params_.max_iterations; ++iter) {
    // 1. Rollout predicted trajectory
    std::vector<Eigen::Vector3d> X(N+1);
    X[0] = current_state;
    for (int k = 0; k < N; ++k) {
      X[k+1] = propagateState(X[k], U[k], params_.dt);
    }

    // 2. Compute total cost
    double cost = 0.0;
    for (int k = 0; k < N; ++k) {
      // Tracking error cost
      double ex = X[k+1](0) - ref_traj[k](0);
      double ey = X[k+1](1) - ref_traj[k](1);
      double et = X[k+1](2) - ref_traj[k](2);
      while (et > M_PI) et -= 2*M_PI;
      while (et < -M_PI) et += 2*M_PI;

      cost += params_.q_x * ex*ex + params_.q_y * ey*ey + params_.q_theta * et*et;

      // Control effort cost
      cost += params_.r_v * U[k](0)*U[k](0) + params_.r_omega * U[k](1)*U[k](1);

      // Control rate cost
      Eigen::Vector2d delta_u = U[k] - (k==0 ? last_control : U[k-1]);
      cost += params_.r_delta_v * delta_u(0)*delta_u(0)
            + params_.r_delta_omega * delta_u(1)*delta_u(1);

      // Obstacle cost
      cost += computeObstacleCost(X[k+1](0), X[k+1](1));
    }

    // 3. Convergence check
    if (std::abs(prev_cost - cost) < params_.convergence_tol) break;
    prev_cost = cost;

    // 4. Compute gradient w.r.t. control inputs (finite difference)
    const double eps = 1e-5;
    std::vector<Eigen::Vector2d> grad_U(N);

    for (int k = 0; k < N; ++k) {
      for (int dim = 0; dim < 2; ++dim) {
        Eigen::Vector2d u_pert = U[k];
        u_pert(dim) += eps;

        std::vector<Eigen::Vector3d> X_pert(N+1);
        X_pert[0] = current_state;
        for (int i = 0; i < N; ++i) {
          if (i == k) X_pert[i+1] = propagateState(X_pert[i], u_pert, params_.dt);
          else X_pert[i+1] = propagateState(X_pert[i], U[i], params_.dt);
        }

        double cost_pert = 0.0;
        for (int i = 0; i < N; ++i) {
          double ex = X_pert[i+1](0) - ref_traj[i](0);
          double ey = X_pert[i+1](1) - ref_traj[i](1);
          double et = X_pert[i+1](2) - ref_traj[i](2);
          while (et > M_PI) et -= 2*M_PI;
          while (et < -M_PI) et += 2*M_PI;
          cost_pert += params_.q_x*ex*ex + params_.q_y*ey*ey + params_.q_theta*et*et;
          cost_pert += params_.r_v*U[i](0)*U[i](0) + params_.r_omega*U[i](1)*U[i](1);
          cost_pert += computeObstacleCost(X_pert[i+1](0), X_pert[i+1](1));
        }

        grad_U[k](dim) = (cost_pert - cost) / eps;
      }
    }

    // 5. Gradient descent update
    for (int k = 0; k < N; ++k) {
      U[k] -= learning_rate * grad_U[k];

      // Clamp to velocity limits
      U[k](0) = std::clamp(U[k](0), -params_.max_vel_x, params_.max_vel_x);
      U[k](1) = std::clamp(U[k](1), -params_.max_vel_theta, params_.max_vel_theta);
    }
  }

  // Return first control input (receding horizon)
  return U[0];
}

// ------------------------------
// Debug visualization
// ------------------------------
void CustomLocalController::publishDebugMarkers(
  const std::vector<Eigen::Vector3d> & predicted_traj,
  const std::vector<Eigen::Vector3d> & ref_traj)
{
  visualization_msgs::msg::MarkerArray ma;
  auto node = node_.lock();
  if (!node) return;
  rclcpp::Time now = node->now();
  std::string frame = costmap_ros_->getGlobalFrameID();

  // Predicted trajectory (red)
  visualization_msgs::msg::Marker pred_m;
  pred_m.header.frame_id = frame;
  pred_m.header.stamp = now;
  pred_m.ns = "predicted_traj";
  pred_m.id = 0;
  pred_m.type = visualization_msgs::msg::Marker::LINE_STRIP;
  pred_m.action = visualization_msgs::msg::Marker::ADD;
  pred_m.scale.x = 0.03;
  pred_m.color.r = 1.0;
  pred_m.color.a = 0.8;
  pred_m.lifetime = rclcpp::Duration::from_seconds(0.1);

  for (const auto & s : predicted_traj) {
    geometry_msgs::msg::Point p;
    p.x = s(0); p.y = s(1); p.z = 0.05;
    pred_m.points.push_back(p);
  }
  ma.markers.push_back(pred_m);

  // Reference trajectory (green)
  visualization_msgs::msg::Marker ref_m = pred_m;
  ref_m.ns = "reference_traj";
  ref_m.id = 1;
  ref_m.color.r = 0.0;
  ref_m.color.g = 1.0;
  ref_m.points.clear();
  for (const auto & s : ref_traj) {
    geometry_msgs::msg::Point p;
    p.x = s(0); p.y = s(1); p.z = 0.05;
    ref_m.points.push_back(p);
  }
  ma.markers.push_back(ref_m);

  marker_pub_->publish(ma);
}

// ------------------------------
// Main control loop entry
// ------------------------------
geometry_msgs::msg::Twist CustomLocalController::computeVelocityCommands(
  const geometry_msgs::msg::PoseStamped & pose,
  const geometry_msgs::msg::Twist & /*velocity*/,
  nav2_core::GoalChecker * /*goal_checker*/)
{
  geometry_msgs::msg::Twist out_twist{};
  if (global_plan_.poses.empty()) return out_twist;

  // 1. Current robot state [x, y, theta]
  Eigen::Vector3d current_state(
    pose.pose.position.x,
    pose.pose.position.y,
    tf2::getYaw(pose.pose.orientation));

  // 2. Generate reference trajectory from global plan
  auto ref_traj = generateReferenceTrajectory(pose);
  if ((int)ref_traj.size() < params_.prediction_horizon) return out_twist;

  // 3. Solve MPC
  Eigen::Vector2d u_opt = solveMpc(current_state, ref_traj, last_control_);

  // 4. Hard safety: critical obstacle distance override
  double rx, ry;
  costmap_ros_->getRobotPose(rx, ry);
  double obs_cost = computeObstacleCost(rx, ry);
  if (obs_cost > params_.w_obstacle * params_.safe_distance * params_.safe_distance * 0.8) {
    RCLCPP_WARN_THROTTLE(logger_, *node_.lock()->get_clock(), 500,
      "Critical obstacle proximity! Emergency stop.");
    out_twist.linear.x = 0.0;
    out_twist.angular.z = 0.0;
    last_control_ = Eigen::Vector2d::Zero();
    return out_twist;
  }

  // 5. Assign output
  out_twist.linear.x = u_opt(0);
  out_twist.angular.z = u_opt(1);
  last_control_ = u_opt;

  // 6. Optional: publish debug markers
  std::vector<Eigen::Vector3d> pred_traj;
  Eigen::Vector3d s = current_state;
  for (int k = 0; k < params_.prediction_horizon; ++k) {
    s = propagateState(s, u_opt, params_.dt);
    pred_traj.push_back(s);
  }
  publishDebugMarkers(pred_traj, ref_traj);

  return out_twist;
}

}  // namespace nav2_custom_planner_controller

// Register as Nav2 plugin
PLUGINLIB_EXPORT_CLASS(
  nav2_custom_planner_controller::CustomLocalController,
  nav2_core::Controller)
