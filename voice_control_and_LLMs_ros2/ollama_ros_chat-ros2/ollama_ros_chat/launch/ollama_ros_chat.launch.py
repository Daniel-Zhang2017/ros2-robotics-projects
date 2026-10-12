from launch import LaunchDescription
from launch_ros.actions import Node
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    # 参数文件路径
    config = os.path.join(
        get_package_share_directory('ollama_ros_chat'),
        'config',
        'ollama_params.yaml'
    )

    return LaunchDescription([
        # 服务器节点
        Node(
            package='ollama_ros_chat',
            executable='chat_service',
            name='chat_service',
            output='screen',
            parameters=[config]             # 加载 yaml
        )
    ])
