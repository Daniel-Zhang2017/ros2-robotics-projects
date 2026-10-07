from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package="status_publisher",
            executable="sys_status_pub",
            name="sys_status_pub"
        ),
        Node(
            package="sys_status_monitor",
            executable="sys_status_sub",
            name="sys_status_sub"
        ),
        Node(
            package="sys_status_monitor",
            executable="battery_alert",
            name="battery_alert"
        ),
         Node(
            package="sys_status_monitor",
            executable="battery_bridge",
            name="battery_bridge"
        )
    ])
