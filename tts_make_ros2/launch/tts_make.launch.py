import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    ld = LaunchDescription()

    tts_dir = get_package_share_directory('tts')
    tts_launch_dir = os.path.join(tts_dir, 'launch')
    tts_config = os.path.join(tts_dir, 'config', 'tts_params.yaml')
    resource_param = {"source_path": tts_dir}

    #合成语音的文本输入
    tts_text = {"tts_text": "你好小微"} 

    tts_make = Node(
        package="tts",
        executable="tts_node",
        output='screen',
        parameters=[resource_param,
            tts_text,
            tts_config]                     
    )

    ld.add_action(tts_make)

    return ld