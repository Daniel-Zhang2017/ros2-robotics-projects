"""
  Copyright 2018 The Cartographer Authors
  Copyright 2022 Wyca Robotics (for the ros2 conversion)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, SetRemap
from launch_ros.substitutions import FindPackageShare
from launch.launch_description_sources import PythonLaunchDescriptionSource
import os
from  ament_index_python.packages import get_package_share_directory

def generate_launch_description():

    ## ***** Launch arguments *****
    use_sim_time_arg = DeclareLaunchArgument('use_sim_time', default_value = 'False')

    ## ***** File paths ******
    #pkg_share = FindPackageShare('cartographer_ros').find('cartographer_ros')
    #urdf_dir = os.path.join(pkg_share, 'urdf')
    #urdf_file = os.path.join(urdf_dir, 'backpack_3d.urdf')
    #with open(urdf_file, 'r') as infp:
    #    robot_desc = infp.read()

    ## ***** Nodes *****
    #robot_state_publisher_node = Node(
    #    package = 'robot_state_publisher',
    #    executable = 'robot_state_publisher',
    #    parameters=[
    #        {'robot_description': robot_desc},
    #        {'use_sim_time': LaunchConfiguration('use_sim_time')}],
    #    output = 'screen'
    #    )
    cartographer_config_dir = DeclareLaunchArgument('cartographer_config_dir', 
        default_value=os.path.join(get_package_share_directory('wheeltec_cartographer') , 'config'))
    configuration_basename = DeclareLaunchArgument('configuration_basename', default_value='carto_3d.lua')
    
    resolution = DeclareLaunchArgument('resolution', default_value='0.05')
    publish_period_sec = DeclareLaunchArgument('publish_period_sec', default_value='0.5')
    
    cartographer_node = Node(
        package = 'cartographer_ros',
        executable = 'cartographer_node',
        parameters = [{'use_sim_time': LaunchConfiguration('use_sim_time')}],
        arguments = [
            '-configuration_directory', LaunchConfiguration('cartographer_config_dir'),
            '-configuration_basename', LaunchConfiguration('configuration_basename')],
        remappings = [
            ('points2', 'point_cloud_raw'),
            ('imu','/imu/data_raw')],
        output = 'screen'
        )

    cartographer_occupancy_grid_node = Node(
        package = 'cartographer_ros',
        executable = 'cartographer_occupancy_grid_node',
        parameters = [
            {'use_sim_time': True},
            {'resolution': 0.05}],
        #remappings=[('odom','odom'),
        #    ('imu','/imu/data')],
        )
        
    bringup_dir = get_package_share_directory('turn_on_wheeltec_robot')
    launch_dir = os.path.join(bringup_dir, 'launch')

    wheeltec_robot = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'turn_on_wheeltec_robot.launch.py')),
            launch_arguments={'carto_slam': 'true'}.items(),
    )
    wheeltec_lidar = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'wheeltec_lidar.launch.py')),
    )
    delayed_carto = TimerAction(
        period=5.0,  
        actions=[
            use_sim_time_arg,
            cartographer_config_dir,
            configuration_basename,
            cartographer_node,
            cartographer_occupancy_grid_node,
        ])
    
    return LaunchDescription([
    	wheeltec_robot,
    	wheeltec_lidar,
        delayed_carto
    ])
