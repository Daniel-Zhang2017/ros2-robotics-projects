import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
import launch_ros.actions

#def launch(launch_descriptor, argv):
def generate_launch_description():
    bringup_dir = get_package_share_directory('turn_on_wheeltec_robot')
    launch_dir = os.path.join(bringup_dir, 'launch')
    gnss_bringup_dir = get_package_share_directory('wheeltec_gps_driver')
    gnss_launch_dir = os.path.join(gnss_bringup_dir, 'launch')
    wheeltec_robot = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'turn_on_wheeltec_robot.launch.py')),
    )
    lidar_ros = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'wheeltec_lidar.launch.py')),
    )
    wheeltec_gnss = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(gnss_launch_dir, 'wheeltec_dual_rtk_driver_nmea.launch.py')),
    )
    return LaunchDescription([
        wheeltec_robot,lidar_ros,wheeltec_gnss,]
    )

