#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.h>

#include <example_interfaces/msg/bool.hpp>
#include <example_interfaces/msg/float64_multi_array.hpp>

#include <my_robot_interfaces/msg/pose_command.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <moveit_msgs/msg/move_it_error_codes.hpp>
#include <moveit_msgs/msg/robot_trajectory.hpp>

#include <tf2/LinearMath/Quaternion.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using MoveGroupInterface =
    moveit::planning_interface::MoveGroupInterface;

using Bool = example_interfaces::msg::Bool;
using FloatArray = example_interfaces::msg::Float64MultiArray;
using PoseCmd = my_robot_interfaces::msg::PoseCommand;

using namespace std::placeholders;

class Commander
{
public:
    Commander(const std::shared_ptr<rclcpp::Node>& node)
        : node_(node)
    {
        // 创建机械臂规划组
        arm_ = std::make_shared<MoveGroupInterface>(node_, "arm");
        arm_->setMaxVelocityScalingFactor(1.0);
        arm_->setMaxAccelerationScalingFactor(1.0);

        // 创建夹爪规划组
        gripper_ = std::make_shared<MoveGroupInterface>(node_, "gripper");

        // 订阅夹爪控制话题
        // true：打开夹爪
        // false：关闭夹爪
        open_gripper_sub_ = node_->create_subscription<Bool>(
            "open_gripper",
            10,
            std::bind(
                &Commander::openGripperCallback,
                this,
                _1));

        // 订阅关节角控制话题
        // 消息中必须包含 6 个关节角度
        joint_cmd_sub_ = node_->create_subscription<FloatArray>(
            "joint_command",
            10,
            std::bind(
                &Commander::jointCmdCallback,
                this,
                _1));

        // 订阅末端位姿控制话题
        // 消息类型：my_robot_interfaces/msg/PoseCommand
        pose_cmd_sub_ = node_->create_subscription<PoseCmd>(
            "pose_command",
            10,
            std::bind(
                &Commander::poseCmdCallback,
                this,
                _1));
    }

    // 前往 MoveIt 中保存的命名姿态，例如 home、pose_1
    void goToNamedTarget(const std::string& name)
    {
        arm_->setStartStateToCurrentState();
        arm_->setNamedTarget(name);

        planAndExecute(arm_);
    }

    // 前往指定的六个关节角度
    void goToJointTarget(const std::vector<double>& joints)
    {
        if (joints.size() != 6)
        {
            RCLCPP_ERROR(
                node_->get_logger(),
                "Joint target requires 6 values, but received %zu.",
                joints.size());
            return;
        }

        arm_->setStartStateToCurrentState();
        arm_->setJointValueTarget(joints);

        planAndExecute(arm_);
    }

    // 前往指定末端位姿
    // cartesian_path = false：普通路径规划
    // cartesian_path = true ：笛卡尔路径规划，末端尽量直线运动
    void goToPoseTarget(
        double x,
        double y,
        double z,
        double roll,
        double pitch,
        double yaw,
        bool cartesian_path = false)
    {
        // 将 RPY 欧拉角转换为四元数
        tf2::Quaternion q;
        q.setRPY(roll, pitch, yaw);
        q.normalize();

        geometry_msgs::msg::PoseStamped target_pose;

        // 通常是 base_link
        target_pose.header.frame_id = arm_->getPlanningFrame();

        target_pose.pose.position.x = x;
        target_pose.pose.position.y = y;
        target_pose.pose.position.z = z;

        target_pose.pose.orientation.x = q.getX();
        target_pose.pose.orientation.y = q.getY();
        target_pose.pose.orientation.z = q.getZ();
        target_pose.pose.orientation.w = q.getW();

        arm_->setStartStateToCurrentState();

        // 普通位姿规划
        if (!cartesian_path)
        {
            arm_->setPoseTarget(target_pose);

            planAndExecute(arm_);

            arm_->clearPoseTargets();
        }
        // 笛卡尔路径规划
        else
        {
            std::vector<geometry_msgs::msg::Pose> waypoints;
            waypoints.push_back(target_pose.pose);

            moveit_msgs::msg::RobotTrajectory trajectory;
            moveit_msgs::msg::MoveItErrorCodes error_code;

            double fraction = arm_->computeCartesianPath(
                waypoints,
                0.01,
                0.0,
                trajectory,
                true,
                &error_code);

            if (fraction >= 0.999)
            {
                auto result = arm_->execute(trajectory);

                if (result != moveit::core::MoveItErrorCode::SUCCESS)
                {
                    RCLCPP_ERROR(
                        node_->get_logger(),
                        "Cartesian path execution failed!");
                }
            }
            else
            {
                RCLCPP_WARN(
                    node_->get_logger(),
                    "Cartesian path only completed %.1f%%.",
                    fraction * 100.0);
            }
        }
    }

    // 打开夹爪
    void openGripper()
    {
        gripper_->setStartStateToCurrentState();
        gripper_->setNamedTarget("gripper_open");

        planAndExecute(gripper_);
    }

    // 关闭夹爪
    void closeGripper()
    {
        gripper_->setStartStateToCurrentState();
        gripper_->setNamedTarget("gripper_closed");

        planAndExecute(gripper_);
    }

private:
    // 规划成功后再执行
    void planAndExecute(
        const std::shared_ptr<MoveGroupInterface>& move_group)
    {
        MoveGroupInterface::Plan plan;

        bool success =
            (move_group->plan(plan) ==
             moveit::core::MoveItErrorCode::SUCCESS);

        if (!success)
        {
            RCLCPP_ERROR(
                node_->get_logger(),
                "Motion planning failed!");
            return;
        }

        auto result = move_group->execute(plan);

        if (result != moveit::core::MoveItErrorCode::SUCCESS)
        {
            RCLCPP_ERROR(
                node_->get_logger(),
                "Trajectory execution failed!");
        }
    }

    // 收到 open_gripper 消息时触发
    void openGripperCallback(const Bool::SharedPtr msg)
    {
        if (msg->data)
        {
            RCLCPP_INFO(
                node_->get_logger(),
                "Opening gripper...");

            openGripper();
        }
        else
        {
            RCLCPP_INFO(
                node_->get_logger(),
                "Closing gripper...");

            closeGripper();
        }
    }

    // 收到 joint_command 消息时触发
    void jointCmdCallback(const FloatArray::SharedPtr msg)
    {
        std::vector<double> joints(
            msg->data.begin(),
            msg->data.end());

        if (joints.size() != 6)
        {
            RCLCPP_WARN(
                node_->get_logger(),
                "joint_command requires 6 values, but received %zu.",
                joints.size());
            return;
        }

        RCLCPP_INFO(
            node_->get_logger(),
            "Received joint command.");

        goToJointTarget(joints);
    }

    // 收到 pose_command 消息时触发
    void poseCmdCallback(const PoseCmd::SharedPtr msg)
    {
        RCLCPP_INFO(
            node_->get_logger(),
            "Received pose command: x=%.3f, y=%.3f, z=%.3f, "
            "roll=%.3f, pitch=%.3f, yaw=%.3f, cartesian=%s",
            msg->x,
            msg->y,
            msg->z,
            msg->roll,
            msg->pitch,
            msg->yaw,
            msg->cartesian_path ? "true" : "false");

        goToPoseTarget(
            msg->x,
            msg->y,
            msg->z,
            msg->roll,
            msg->pitch,
            msg->yaw,
            msg->cartesian_path);
    }

private:
    std::shared_ptr<rclcpp::Node> node_;

    std::shared_ptr<MoveGroupInterface> arm_;
    std::shared_ptr<MoveGroupInterface> gripper_;

    rclcpp::Subscription<Bool>::SharedPtr open_gripper_sub_;
    rclcpp::Subscription<FloatArray>::SharedPtr joint_cmd_sub_;
    rclcpp::Subscription<PoseCmd>::SharedPtr pose_cmd_sub_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rclcpp::Node>("commander");

    // MoveIt 执行、话题回调可同时处理
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);

    std::thread spinner([&executor]()
    {
        executor.spin();
    });

    auto commander = std::make_shared<Commander>(node);

    RCLCPP_INFO(
        node->get_logger(),
        "Commander is ready. Waiting for open_gripper, "
        "joint_command and pose_command topics...");

    while (rclcpp::ok())
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
    }

    executor.cancel();
    spinner.join();

    rclcpp::shutdown();

    return 0;
}
