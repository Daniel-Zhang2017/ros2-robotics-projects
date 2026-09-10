#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include "turn_on_wheeltec_robot/Quaternion_Solution.h"

//extern "C" {
//#include "turn_on_wheeltec_robot/Quaternion_Solution.h"
//}

// Returns true if |val1| < val2
bool abslt(const double& val1, const double& val2)
{
  return std::abs(val1) < val2;
}

class ImuProcessor : public rclcpp::Node
{
public:
    ImuProcessor() : Node("imu_processor")
    {
        imu_sub_ = create_subscription<sensor_msgs::msg::Imu>("/imu/data_raw", 2,std::bind(&ImuProcessor::imuCallback, this, std::placeholders::_1));
        odom_sub_ = create_subscription<nav_msgs::msg::Odometry>("/odom", 2,std::bind(&ImuProcessor::odomCallback, this, std::placeholders::_1));
        imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("/imu/data_filtered", 2);
    }

private:
    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        linear_vel_ = msg->twist.twist.linear.x;
        angular_vel_ = msg->twist.twist.angular.z;
        odom_vaild=true;

    }

    void imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if(!odom_vaild) return;

       //RCLCPP_INFO(this->get_logger(), "linear_vel: %f, angular_vel: %f", linear_vel_, angular_vel_);

        bool is_static = abslt(linear_vel_,x_vel_threshold) && abslt(angular_vel_,z_vel_threshold);
        
        if(is_static){
            static_count++;
            dynamic_count=0;
        }
        else{
            dynamic_count++;
            static_count=0;
        }
        imu_freeze = abslt(static_threshold,static_count);

        //imu_freeze = is_static ? 1 : 0;
        //姿态解算
        Quaternion_Solution(
            msg->angular_velocity.x,
            msg->angular_velocity.y,
            msg->angular_velocity.z,
            msg->linear_acceleration.x,
            msg->linear_acceleration.y,
            msg->linear_acceleration.z
        );

        sensor_msgs::msg::Imu out = *msg;

        //使用Quaternion_Solution中的全局姿态
        out.orientation.w = q0;
        out.orientation.x = q1;
        out.orientation.y = q2;
        out.orientation.z = q3;

        imu_pub_->publish(out);
    }

    double linear_vel_ = 0.0;
    double angular_vel_ = 0.0;

    double x_vel_threshold=0.05;
    double z_vel_threshold=0.05;

    bool odom_vaild=false;
    int static_count=0;
    int dynamic_count=0;
    int static_threshold=10;
    int dynamic_threshold=3;


    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv); 
  auto node = std::make_shared<ImuProcessor>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;  
} 
