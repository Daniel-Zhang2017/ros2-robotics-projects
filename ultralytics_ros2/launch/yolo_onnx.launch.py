import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    model_dir = os.path.join(get_package_share_directory('ultralytics_ros2'), 'model')

    return LaunchDescription([
        Node(
            package='ultralytics_ros2',
            executable='detection_node_onnx',
            name='yolo_detector_onnx',
            output='screen',
            parameters=[{
                'model': os.path.join(model_dir, 'yolov8n.pt'),
                'input_image_topic': '/image_raw',
                'enable_cuda': True,
                'conf_threshold': 0.5,
                'infer_period': 0.08,
                'export_onnx': True,
                'onnx_export_model': os.path.join(model_dir, 'yolov8n.pt'),
                'onnx_export_imgsz': 640,
                'onnx_export_opset': 12,
            }]
        )
    ])
