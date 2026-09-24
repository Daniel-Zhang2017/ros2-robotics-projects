from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ultralytics_ros2',
            executable='detection_node',
            name='yolo_detector',
            parameters=[
                {'model': '/home/da/ros2_ws/src/ultralytics_ros2/model/yolov8s.pt'}, #replace with your own mode pt file
                {'input_image_topic': '/image_raw'},
                {'enable_cuda': True},
                {'conf_threshold': 0.5}
            ]
        )
    ])
