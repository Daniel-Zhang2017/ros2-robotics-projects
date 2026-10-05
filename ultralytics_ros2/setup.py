import os
import glob
from setuptools import setup

package_name = 'ultralytics_ros2'

# Get absolute path to model folder inside this package source
pkg_model_dir = os.path.join(os.path.dirname(__file__), "model")
pt_files = glob.glob(os.path.join(pkg_model_dir, "*.pt"))

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # Copy all *.pt from source model/ to install/share/ultralytics_ros2/model
        (os.path.join('share', package_name, 'model'), pt_files),
        ('share/' + package_name + '/launch', ['launch/yolo.launch.py']),
        ('share/' + package_name + '/launch', ['launch/yolo_onnx.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Your Name',
    maintainer_email='your@email.com',
    description='YOLOv8 object detection package for ROS2',
    license='Apache License 2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'detection_node = ultralytics_ros2.detection_node:main',
            'detection_node_onnx = ultralytics_ros2.detection_node_onnx:main',
        ],
    },
)
