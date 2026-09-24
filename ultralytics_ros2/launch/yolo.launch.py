from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ultralytics_ros2',
            executable='detection_node',
            name='yolo_detector',
            parameters=[
                {'model': '/home/wheeltec/wheeltec_ros2/src/ultralytics_ros2/model/yolov8s.pt'},
                {'input_image_topic': '/image_raw'},
                {'enable_cuda': True},
                {'conf_threshold': 0.5}
            ]
        )
    ])
