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
            name='body_process',
            output='screen'
        ),

        # 动作驱动小车运动节点
        Node(
            package='bodyreader',
            executable='interaction',
            name='body_interaction'
        ),

        # 反馈（语音反馈/图像UI）
        Node(
            package='bodyreader',
            executable='feedback',
            name='body_feedback',
            output='screen',
            parameters=[
                {'voice_feedback': True},
                {'interaction': 1}
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

        # 注释掉的 USB 摄像头启动文件
        # IncludeLaunchDescription(
        #     FrontendLaunchDescriptionSource(
        #         PathJoinSubstitution([FindPackageShare('usb_cam'), 'launch', 'usb_cam-test.launch'])
        #     )
        # )
    ])

