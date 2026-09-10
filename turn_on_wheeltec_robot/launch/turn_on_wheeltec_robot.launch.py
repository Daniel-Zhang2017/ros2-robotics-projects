import os
from pathlib import Path
import launch
from launch.actions import SetEnvironmentVariable
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import PushRosNamespace
import launch_ros.actions
from launch.conditions import IfCondition
from launch.conditions import UnlessCondition


def generate_launch_description():
    # Get the launch directory
    bringup_dir = get_package_share_directory('turn_on_wheeltec_robot')
    launch_dir = os.path.join(bringup_dir, 'launch')

    carto_slam = LaunchConfiguration('carto_slam', default='False')
    carto_slam_dec = DeclareLaunchArgument('carto_slam',default_value='False')

    robot_nav = LaunchConfiguration('robot_nav', default='False')
    robot_nav_dec = DeclareLaunchArgument('robot_nav',default_value='False')
    start_state_publisher = LaunchConfiguration(
        'start_state_publisher',
        default='true'
    )

    start_robot_state_publisher_dec = DeclareLaunchArgument(
        'start_state_publisher',
        default_value='true',
        description='是否启动 robot_state_publisher'
    )

    imu_config = Path(get_package_share_directory('turn_on_wheeltec_robot'), 'config', 'imu.yaml')
                    
    wheeltec_robot = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'base_serial.launch.py')),
    )

    robot_ekf = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'wheeltec_ekf.launch.py')),
            launch_arguments={'carto_slam': carto_slam,'robot_nav':robot_nav}.items(),            
    )                                                       
                           

    

    car_mode_type = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(launch_dir, 'robot_mode_description.launch.py')
        ),
        launch_arguments={
            'start_state_publisher': start_state_publisher,
        }.items(),
    )

    ld = LaunchDescription()
    ld.add_action(carto_slam_dec)
    ld.add_action(robot_nav_dec)
    ld.add_action(start_robot_state_publisher_dec)
    ld.add_action(wheeltec_robot)
    ld.add_action(robot_ekf)
    ld.add_action(car_mode_type)
    return ld

