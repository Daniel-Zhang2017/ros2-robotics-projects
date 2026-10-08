import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # 初始化小车底盘
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(get_package_share_directory('turn_on_wheeltec_robot'), 'launch', 'turn_on_wheeltec_robot.launch.py')
            )
        ),
        
        # 人体骨架识别节点
        Node(
            package='bodyreader',
            executable='main',
            name='body_main',
            output='screen',
            parameters=[
                {'rgb_stream': True},
                {'body_stream': True}
            ]
        ),
        
        # 动作定义节点
        Node(
            package='bodyreader',
            executable='bodydata_process',
            name='bodydata_process',
            output='screen'
        ),
        
        # 跟随节点
        Node(
            package='bodyreader',
            executable='follower',
            name='body_follower',
            parameters=[
                {'bodyfollow_x_p': 0.5},
                {'bodyfollow_x_d': 0.33},
                {'bodyfollow_z_p': 4.0},
                {'bodyfollow_z_d': 15.0},
                {'mode': 2}
            ]
        ),
        
        # 反馈（语音反馈/图像UI）
        Node(
            package='bodyreader',
            executable='feedback',
            name='body_feedback',
            output='screen',
            parameters=[
                {'voice_feedback': True},
                {'interaction': 0}
            ]
        ),
        
        # 显示节点
        Node(
            package='bodyreader',
            executable='display.py',
            name='body_display'
        ),
        
        # 图像传输节点
        Node(
            package='bodyreader',
            executable='compressed.py',
            name='compressed',
            parameters=[
                {'input_image_topic': '/body/body_display'},
                {'output_image_topic': '/repub/body/body_display/compressed'}
            ]
        ),
    ])


