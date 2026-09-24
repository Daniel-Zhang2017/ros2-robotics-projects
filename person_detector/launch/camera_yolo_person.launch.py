from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    video_device = LaunchConfiguration('video_device')
    video_device_arg = DeclareLaunchArgument(
        'video_device',
        default_value='/dev/video0',
        description='USB camera device path'
    )

    usb_cam_dir = get_package_share_directory('usb_cam')
    params_path = os.path.join(usb_cam_dir, 'config', 'params.yaml')

    # 1. USB camera node
    usb_cam_node = Node(
        package='usb_cam',
        executable='usb_cam_node_exe',
        output='screen',
        name='usb_cam',
        parameters=[
            params_path,
            {'video_device': video_device}
        ]
    )

    # 2. YOLO detection node (ultralytics_ros2)
    yolo_node = Node(
        package='ultralytics_ros2',
        executable='detection_node',
        name='yolo_detector',
        parameters=[
            {'model': '/home/da/ros2_ws/src/ultralytics_ros2/model/yolov8s.pt'},
            {'input_image_topic': '/image_raw'},
            {'enable_cuda': True},
            {'conf_threshold': 0.5},
            {'infer_period': 0.08}
        ]
    )

    # 3. Person detector node (this package person_detector)
    person_detector_node = Node(
        package='person_detector',
        executable='person_detector_node',
        name='person_detector',
        output='screen'
    )

    ld = LaunchDescription()
    ld.add_action(video_device_arg)
    ld.add_action(usb_cam_node)
    ld.add_action(yolo_node)
    ld.add_action(person_detector_node)
    return ld
