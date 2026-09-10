import os
from pathlib import Path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    pkg_dir = get_package_share_directory('turn_on_wheeltec_robot')

    ekf_config = Path(pkg_dir, 'config', 'ekf.yaml')
    ekf_carto_config = Path(pkg_dir, 'config', 'ekf_carto.yaml')
    ekf_nav_config = Path(pkg_dir, 'config', 'ekf_nav.yaml')

    carto_slam = LaunchConfiguration('carto_slam')
    robot_nav = LaunchConfiguration('robot_nav')

    carto_slam_dec = DeclareLaunchArgument('carto_slam',default_value='false')
    robot_nav_dec = DeclareLaunchArgument('robot_nav',default_value='false')
    
    log_args = LogInfo(msg=['[ekf launch] carto_slam=', carto_slam, ', robot_nav=', robot_nav])

    def launch_ekf(context, *args, **kwargs):
        carto_slam_val = LaunchConfiguration('carto_slam').perform(context)
        robot_nav_val = LaunchConfiguration('robot_nav').perform(context)
        carto_slam_bool = carto_slam_val.lower() == 'true'
        robot_nav_bool = robot_nav_val.lower() == 'true'

        nodes = []

        if carto_slam_bool:
            nodes.append(
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='carto_ekf_filter_node',
                    parameters=[ekf_carto_config],
                    remappings=[
                        ('/odometry/filtered', 'odom_combined')
                    ]
                )
            )

        elif robot_nav_bool:
            nodes.append(
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='nav_ekf_filter_node',
                    parameters=[ekf_nav_config],
                    remappings=[
                        ('/odometry/filtered', 'odom_combined')
                    ]
                )
            )

        else:
            nodes.append(
                Node(
                    package='robot_localization',
                    executable='ekf_node',
                    name='ekf_filter_node',
                    parameters=[ekf_config],
                    remappings=[
                        ('/odometry/filtered', 'odom_combined')
                    ]
                )
            )

        return nodes

    return LaunchDescription([
        carto_slam_dec,
        robot_nav_dec,
        log_args,
        OpaqueFunction(function=launch_ekf),
    ])