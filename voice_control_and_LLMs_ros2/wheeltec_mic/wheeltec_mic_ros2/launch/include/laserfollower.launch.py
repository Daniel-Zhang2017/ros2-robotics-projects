from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable)
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
import launch_ros.actions

#def launch(launch_descriptor, argv):
def generate_launch_description():

    return LaunchDescription([
        launch_ros.actions.Node(
            package='simple_follower_ros2', 
            executable='laserfollower', 
            ),]
    )
