import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('ultralytics_ros2')
    model_dir = os.path.join(pkg_share, 'model')
    # Direct absolute path to your source pt file (local debug only)
    model_path = '/home/da/ros2_ws/src/ultralytics_ros2/model/yolov8s.pt'

    return LaunchDescription([
        Node(
            package='ultralytics_ros2',
            executable='detection_node_onnx',
            name='yolo_detector_onnx',
            output='screen',
            parameters=[{
                'model': model_path,
                'input_image_topic': '/image_raw',
                'enable_cuda': True,
                'conf_threshold': 0.5,
                'infer_period': 0.08,
                'export_onnx': False,
                'onnx_export_model': model_path,
                'onnx_export_imgsz': 640,
                'onnx_export_opset': 12,
                'enable_half': False,
            }],
        )
    ])
