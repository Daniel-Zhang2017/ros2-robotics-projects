import os, yaml
from pathlib import Path
import launch_ros.actions
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable, OpaqueFunction)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.launch_description_sources import AnyLaunchDescriptionSource
from launch_ros.parameter_descriptions import ParameterFile
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node

def load_yaml(file_path: Path) -> dict:
    with open(file_path, 'r') as f:
        return yaml.safe_load(f)

def spawn_camera_nodes(context, *args, **kwargs):
    # 1. 取 yaml 文件路径
    param_file = LaunchConfiguration('camera_modes_yaml').perform(context)
    cfg = load_yaml(Path(param_file))

    # 2. 决定使用哪个相机
    camera_mode_ = LaunchConfiguration('camera_mode').perform(context) or cfg['camera_mode']
    print(f'camera_mode:{camera_mode_}')
    file_name = f'{camera_mode_}.launch.xml'

    # 3. 构造相机节点
    astra_dir = get_package_share_directory('astra_camera')
    astra_launch_dir = os.path.join(astra_dir, 'launch')
    
    usbcam_dir = get_package_share_directory('usb_cam')
    usbcam_launch_dir = os.path.join(usbcam_dir, 'launch')
    
    usbcam_arg = DeclareLaunchArgument(
        'video_device', default_value='/dev/video0',
        description='video device serial number.'
    )

    if camera_mode_.startswith('astra') or camera_mode_.startswith('dabai') or camera_mode_.startswith('gemini'):
        camera_launch = IncludeLaunchDescription(
            AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, file_name)),
            launch_arguments=[('enable_d2c_viewer', 'True')]
        )
    elif camera_mode_.startswith('usb'):
        camera_launch = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(usbcam_launch_dir, 'demo.launch.py')),
            launch_arguments={'video_device': '/dev/RgbCam'}.items()
        )

    actions = []
    actions.append(camera_launch)

    compress_color = Node(
        package='image_transport',
        executable='republish',
        name='republish_color_compressed',
        arguments=['raw', 'compressed'],
        remappings=[
            ('in', '/camera/color/image_raw'),
            ('out/compressed', '/camera/color/image_raw/compressed')  
        ],
        parameters=[{
            'format': 'jpeg',
            'jpeg_quality': 85  # 0~100，值越大越清晰但带宽占用越高
        }]
    )

    compress_depth = Node(
        package='image_transport',
        executable='republish',
        name='republish_depth_compressed',
        arguments=['raw', 'compressedDepth'],
        remappings=[
            ('in', '/camera/depth/image_raw'),
            ('out/compressedDepth', '/camera/depth/image_raw/compressed')  
        ],
        output='screen'
    )

    actions.append(compress_color)
    actions.append(compress_depth)
    return actions

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'camera_modes_yaml',
            default_value=os.path.join(
                get_package_share_directory('turn_on_wheeltec_robot'),
                'config', 'wheeltec_param.yaml'),
            description='Path to camera_modes.yaml'),
        DeclareLaunchArgument(
            'camera_mode',
            default_value='',   # 空则使用 yaml 内默认值
            description='Which camera mode to launch'),
        OpaqueFunction(function=spawn_camera_nodes),
    ])
