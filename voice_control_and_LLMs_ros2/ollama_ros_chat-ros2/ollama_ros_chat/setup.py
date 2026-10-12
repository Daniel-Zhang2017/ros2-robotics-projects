from setuptools import find_packages, setup
import os 
from glob import glob
package_name = 'ollama_ros_chat'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name,'config'),glob("config/*.yaml")),
        (os.path.join('share', package_name,'launch'),glob("launch/*launch.[pxy][yma]*"))
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='wheeltec',
    maintainer_email='wheeltec@todo.todo',
    description='TODO: ROS2 package for Ollama chat interface',
    license='Apache License 2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'topic_server = ollama_ros_chat.ollama_topic_server:main',
            'topic_client = ollama_ros_chat.ollama_topic_client:main',
            'chat_service = ollama_ros_chat.ollama_service:main',
            'chat_client = ollama_ros_chat.ollama_client:main',           
        ],
    },
)
