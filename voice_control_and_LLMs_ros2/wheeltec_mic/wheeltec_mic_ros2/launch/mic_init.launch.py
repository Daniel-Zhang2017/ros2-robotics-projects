import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    mode_arg = DeclareLaunchArgument(
        'use_mode',
        default_value='M2',  
        description='选择使用的麦克风模块类型: M2, NEW_M2, M07'
    )
    use_mode_ = LaunchConfiguration('use_mode')
    
    # 获取功能包的共享目录
    pkg_share_dir = get_package_share_directory('wheeltec_mic_ros2')
    audio_save_path = os.path.join(pkg_share_dir, 'audio')
    
    wheeltec_mic = Node(
        package="wheeltec_mic_ros2",
        executable="wheeltec_mic",
        output='screen',
        parameters=[{"usart_port_name": "/dev/wheeltec_mic",
                    "serial_baud_rate": 115200}],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M2"']))
    )

    wheeltec_m2n = Node(
        package="wheeltec_mic_ros2",
        executable="wheeltec_m2n",
        output='screen',
        parameters=[{"usart_port_name": "/dev/wheeltec_mic",
                    "virtual_usart_port_name": "/dev/wheeltec_mic_virtual",
                    "serial_baud_rate": 115200,
                    "audio_path": audio_save_path}], 
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "NEW_M2"']))
    )

    wheeltec_m07 = Node(
        package="wheeltec_mic_ros2",
        executable="wheeltec_m07",
        output='screen',
        parameters=[{"usart_port_name": "/dev/wheeltec_mic",
                    "serial_baud_rate": 115200}], 
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M07"']))
    )

    voice_control = Node(
        package="wheeltec_mic_ros2",
        executable="voice_control",
        output='screen',
        parameters=[{"appid": "6159904a",
                    "record_device_name": "hw:CARD=XFMDPV0018,DEV=0"}],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M2"']))
    )

    voice_control_m2n = Node(
        package="wheeltec_mic_ros2",
        executable="voice_control",
        output='screen',
        parameters=[{"appid": "6159904a",
                    "record_device_name": "hw:CARD=L6Microphone,DEV=0"}],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "NEW_M2"']))
    )

    voice_control_m07 = Node(
        package="wheeltec_mic_ros2",
        executable="voice_control",
        output='screen',
        parameters=[{"appid": "6159904a",
                    "record_device_name": "hw:CARD=Device,DEV=0",
                    "device_type": use_mode_}],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M07"']))
    )

    ld = LaunchDescription()
    ld.add_action(mode_arg)
    ld.add_action(wheeltec_mic)
    ld.add_action(wheeltec_m2n)
    ld.add_action(wheeltec_m07)
    ld.add_action(voice_control)
    ld.add_action(voice_control_m2n)
    ld.add_action(voice_control_m07)
    
    return ld