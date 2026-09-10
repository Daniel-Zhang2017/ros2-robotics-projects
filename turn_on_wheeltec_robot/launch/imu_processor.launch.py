import os
from launch import LaunchDescription
import launch_ros.actions

def generate_launch_description():
    imu_processor = launch_ros.actions.Node(
            package='turn_on_wheeltec_robot', 
            executable='ImuProcessor', 
            name='imu_processor',
            #output='screen',
    )
    return LaunchDescription([
        imu_processor
    ])

