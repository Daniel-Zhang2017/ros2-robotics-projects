#ifndef NAV2_CUSTOM_PLANNER_CONTROLLER__CUSTOM_LOCAL_CONTROLLER_HPP_
#define NAV2_CUSTOM_PLANNER_CONTROLLER__CUSTOM_LOCAL_CONTROLLER_HPP_

#include <string>
#include <vector>
#include <memory>
#include <Eigen/Dense>

#include "nav2_core/controller.hpp"
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "visualization_msgs/msg/marker_array.hpp"
#include "nav2_costmap_2d/costmap_2d_ros.hpp"
#include "tf2_ros/buffer.h"

namespace nav2_custom_planner_controller
{

struct MpcParams
{
  // Prediction & control horizon
  int prediction_horizon;      // N steps
  double dt;                   // time step per prediction [s]
  int max_iterations;          // max optimization iterations
  double convergence_tol;      // gradient convergence threshold

  // Tracking cost weights
  double q_x;
  double q_y;
  double q_theta;

  // Control effort weights
  double r_v;
  double r_omega;

  // Control rate (delta) weights (smoothness)
  double r_delta_v;
  double r_delta_omega;

  // Obstacle avoidance cost
  double w_obstacle;
  double safe_distance;        // desired clearance [m]
  double critical_distance;    // hard stop threshold [m]

  // Velocity limits
  double max_vel_x;
  double max_vel_theta;
  double max_acc_x;
  double max_acc_theta;
};

class CustomLocalController : public nav2_core::Controller
{
public:
  CustomLocalController() = default;
  ~CustomLocalController() override = default;

  void configure(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
    std::string name,
    std::shared_ptr<tf2_ros::Buffer> tf,
    std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros) override;

  void cleanup() override;
  void activate() override;
  void deactivate() override;

  geometry_msgs::msg::Twist computeVelocityCommands(
    const geometry_msgs::msg::PoseStamped & pose,
    const geometry_msgs::msg::Twist & velocity,
    nav2_core::GoalChecker * goal_checker) override;

  void setPlan(const nav_msgs::msg::Path & path) override;

protected:
  // Generate reference trajectory from global plan for MPC horizon
  std::vector<Eigen::Vector3d> generateReferenceTrajectory(
    const geometry_msgs::msg::PoseStamped & current_pose);

  // Core MPC solver (gradient-based unconstrained + soft obstacle cost)
  Eigen::Vector2d solveMpc(
    const Eigen::Vector3d & current_state,
    const std::vector<Eigen::Vector3d> & ref_traj,
    const Eigen::Vector2d & last_control);

  // Differential drive kinematics: predict next state
  Eigen::Vector3d propagateState(
    const Eigen::Vector3d & state,
    const Eigen::Vector2d & control,
    double dt);

  // Compute obstacle cost for a single (x,y) world position
  double computeObstacleCost(double wx, double wy);

  // Publish debug markers (predicted trajectory, repulsion direction)
  void publishDebugMarkers(
    const std::vector<Eigen::Vector3d> & predicted_traj,
    const std::vector<Eigen::Vector3d> & ref_traj);

  rclcpp_lifecycle::LifecycleNode::WeakPtr node_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  std::string plugin_name_;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros_;
  nav2_costmap_2d::Costmap2D * costmap_;

  nav_msgs::msg::Path global_plan_;
  MpcParams params_;
  Eigen::Vector2d last_control_;

  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;
  rclcpp::Logger logger_{rclcpp::get_logger("CustomMpcController")};
};

}  // namespace nav2_custom_planner_controller

#endif  // NAV2_CUSTOM_PLANNER_CONTROLLER__CUSTOM_LOCAL_CONTROLLER_HPP_
