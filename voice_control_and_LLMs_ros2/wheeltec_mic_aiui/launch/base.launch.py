import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    aiui_dir = get_package_share_directory('wheeltec_mic_aiui')
    aiui_launch_dir = os.path.join(aiui_dir, 'launch')
    aiui_include_dir = os.path.join(aiui_launch_dir, 'include')
    command_config = os.path.join(aiui_dir, 'config', 'param.yaml')

    bringup_dir = get_package_share_directory('turn_on_wheeltec_robot')
    launch_dir = os.path.join(bringup_dir, 'launch')
    wheeltec_robot = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'turn_on_wheeltec_robot.launch.py')),
    )
    wheeltec_lidar = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'wheeltec_lidar.launch.py')),
    )

    wheeltec_nav = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(aiui_include_dir, 'voi_navigation.launch.py')),
    )

    command_recognition = Node(
        package="wheeltec_mic_aiui",
        executable="command_recognition",
        output='screen', 
        parameters=[command_config]                   
    )

    node_feedback = Node(
        package="wheeltec_mic_aiui",
        executable="node_feedback",
        #output='screen',
    )

    motion_control = Node(
        package="wheeltec_mic_aiui",
        executable="motion_control",
        #output='screen',
        parameters=[command_config]   
    )

    lasertracker = Node(
        package="simple_follower_ros2", 
        executable="lasertracker", 
        name='lasertracker'
    )

    ld = LaunchDescription()

    ld.add_action(wheeltec_robot)
    ld.add_action(wheeltec_lidar)
    ld.add_action(wheeltec_nav)
    ld.add_action(lasertracker)

    ld.add_action(command_recognition)
    ld.add_action(node_feedback)
    ld.add_action(motion_control)
    
    return ld