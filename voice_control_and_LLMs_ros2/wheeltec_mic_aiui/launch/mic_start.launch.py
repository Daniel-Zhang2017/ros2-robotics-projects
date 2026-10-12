import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.conditions import IfCondition
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    mode_arg = DeclareLaunchArgument(
        'use_mode',
        default_value='NEW_M2',  
        description='选择使用的麦克风模块类型: M07, M2, NEW_M2'
    )
    use_mode_ = LaunchConfiguration('use_mode')

    # 获取ROS包的共享目录
    pkg_share_dir = get_package_share_directory('wheeltec_mic_aiui')
    audio_save_path = os.path.join(pkg_share_dir, 'audio')

    # M07 模块节点
    wheeltec_m07 = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltecM07",
        output='screen',
        parameters=[{
            "usart_port_name": "/dev/wheeltec_mic",
            "serial_baud_rate": 115200
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M07"']))
    )

    # M2 模块节点
    wheeltec_mic = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltec_mic",
        output='screen',
        parameters=[{
            "usart_port_name": "/dev/wheeltec_mic",
            "serial_baud_rate": 115200
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M2"']))
    )

    # NEW_M2 模块节点
    wheeltec_m2n = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltecM2N",
        output='screen',
        parameters=[{
            "usart_port_name": "/dev/wheeltec_mic",
            "virtual_usart_port_name": "/dev/wheeltec_mic_virtual",
            "serial_baud_rate": 115200,
            "audio_path": audio_save_path
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "NEW_M2"']))
    )

    # AIUI 节点 - M07 版本
    wheeltec_mic_aiui_m07 = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltec_mic_aiui",
        output='screen',
        parameters=[{
            "record_device_name": "hw:CARD=Device,DEV=0"  # M07设备
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M07"']))
    )

    # AIUI 节点 - M2 版本
    wheeltec_mic_aiui_m2 = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltec_mic_aiui",
        output='screen',
        parameters=[{
            "record_device_name": "hw:CARD=XFMDPV0018,DEV=0"  # M2设备
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "M2"']))
    )

    # AIUI 节点 - NEW_M2 版本
    wheeltec_mic_aiui_m2n = Node(
        package="wheeltec_mic_aiui",
        executable="wheeltec_mic_aiui",
        output='screen',
        parameters=[{
            "record_device_name": "hw:CARD=L6Microphone,DEV=0"  # M2N设备
        }],
        condition=IfCondition(PythonExpression(['"', use_mode_, '" == "NEW_M2"']))
    )

    ld = LaunchDescription()
    
    ld.add_action(mode_arg)
    
    ld.add_action(wheeltec_m07)
    ld.add_action(wheeltec_mic)
    ld.add_action(wheeltec_m2n)
    ld.add_action(wheeltec_mic_aiui_m07)
    ld.add_action(wheeltec_mic_aiui_m2)
    ld.add_action(wheeltec_mic_aiui_m2n)

    return ld