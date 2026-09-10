import os
import yaml
from pathlib import Path
from ament_index_python.packages import get_package_share_directory
import launch_ros.actions
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, SetEnvironmentVariable, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import LoadComposableNodes
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode
from nav2_common.launch import RewrittenYaml
from launch_ros.parameter_descriptions import ParameterFile
from launch_ros.parameter_descriptions import ParameterValue
from launch.substitutions import Command

def load_yaml(file_path: Path) -> dict:
    """加载YAML配置文件"""
    with open(file_path, 'r') as f:
        return yaml.safe_load(f)


def spawn_robot_nodes(context, *args, **kwargs):
    """
    真正构造节点的函数，由 OpaqueFunction 调用
    根据参数决定是否启动robot_state_publisher
    """
    # ========== 1. 读取配置文件 ==========
    param_file = LaunchConfiguration('wheeltec_param_yaml').perform(context)
    robot_model_file = LaunchConfiguration('robot_model_yaml').perform(context)
    cfg = load_yaml(Path(param_file))
    model_cfg = load_yaml(Path(robot_model_file))

    # ========== 2. 获取车型配置 ==========
    car_mode = LaunchConfiguration('car_mode').perform(context) or cfg['car_mode']
    print(f'[robot_mode_description] 车型: {car_mode}')

    if car_mode not in model_cfg['robot_model']:
        raise ValueError(f'未知车型 "{car_mode}"，请检查 {param_file} 和 {robot_model_file}')

    model_cfg = model_cfg['robot_model'][car_mode]

    # ========== 3. 获取robot_state_publisher启动参数 ==========
    start_state_publisher = LaunchConfiguration('start_state_publisher').perform(context) == 'true'
    print(f'[robot_mode_description] 启动robot_state_publisher: {start_state_publisher}')

    # ========== 4. 构造URDF文件路径 ==========
    urdf_path = os.path.join(
        get_package_share_directory('wheeltec_robot_urdf'),
        'urdf',
        f'{car_mode}_robot.urdf'
    )

    # ========== 5. 构造节点列表 ==========
    actions = []

    # --- 5.1 robot_state_publisher（条件启动）---
    if start_state_publisher:
        if os.path.exists(urdf_path):
            actions.append(
                Node(
                    package='robot_state_publisher',
                    executable='robot_state_publisher',
                    name='robot_state_publisher',
                    parameters=[{
                        'robot_description': ParameterValue(
                            Command(['xacro ', urdf_path]),
                            value_type=str
                        )
                    }],
                ),
            )
            actions.append(
                Node(
                    package='joint_state_publisher',
                    executable='joint_state_publisher',
                    name='joint_state_publisher',
                )
            )
        else:
            print(f'[robot_mode_description] URDF文件不存在')
            
    # --- 5.2 静态TF发布---
    # 静态TF配置： { 'YAML配置键 (兼节点名)' : 'child_frame_id' }
    static_transforms = {
        'base_to_laser':  'laser',
        'base_to_camera': 'camera_link',
        'base_to_link':   'base_link',
        'base_to_gyro':   'gyro_link',
        'base_to_radar':  'radar',
    }
    parent_frame = 'base_footprint'
    # 使用字典 items() 批量生成节点
    for cfg_key, child_frame in static_transforms.items():
        if cfg_key in model_cfg:
            tf_data = model_cfg[cfg_key]
            actions.append(
                Node(
                    package='tf2_ros',
                    executable='static_transform_publisher',
                    name=cfg_key,  # 直接复用 cfg_key 作为节点名，消除冗余
                    arguments=[
                        '--x', str(tf_data[0]),
                        '--y', str(tf_data[1]),
                        '--z', str(tf_data[2]),
                        '--yaw', str(tf_data[3]),
                        '--pitch', str(tf_data[4]),
                        '--roll', str(tf_data[5]),
                        '--frame-id', parent_frame,
                        '--child-frame-id', child_frame,
                    ],
                )
            )
        else:
            print(f'[robot_mode_description] 警告: {robot_model_file} 中缺少 {cfg_key} 配置，已跳过 {child_frame} 的 TF 发布')
    return actions


# ========== Launch 描述 ==========
def generate_launch_description():
    """
    生成launch描述
    支持参数：
    - wheeltec_param_yaml: wheeltec参数配置文件路径
    - robot_model_yaml: 机器人模型配置文件路径
    - car_mode: 车型选择（可选，默认使用yaml中的配置）
    - start_state_publisher: 是否启动robot_state_publisher（默认true）
    """
    return LaunchDescription([
        # ========== 参数声明 ==========

        # 参数1: wheeltec配置文件路径
        DeclareLaunchArgument(
            'wheeltec_param_yaml',
            default_value=os.path.join(
                get_package_share_directory('turn_on_wheeltec_robot'),
                'config', 'wheeltec_param.yaml'
            ),
            description='wheeltec_param.yaml配置文件路径'
        ),

        # 参数2: 机器人模型配置文件路径
        DeclareLaunchArgument(
            'robot_model_yaml',
            default_value=os.path.join(
                get_package_share_directory('turn_on_wheeltec_robot'),
                'config', 'robot_model.yaml'
            ),
            description='robot_model.yaml配置文件路径'
        ),

        # 参数3: 车型选择（可选）
        DeclareLaunchArgument(
            'car_mode',
            default_value='',   # 空字符串表示使用 yaml 里的默认值
            description='车型选择 (mini_akm, mini_mec, S300等)'
        ),
 
        # 参数4: 是否启动robot_state_publisher,joint_state_publisher
        DeclareLaunchArgument(
            'start_state_publisher',
            default_value='true',
            description='是否启动robot_state_publisher,joint_state_publisher（默认true；与机械臂联用时设为false）'
        ),

        # ========== 用 OpaqueFunction 在运行时解析 yaml 并构造节点 ==========
        OpaqueFunction(function=spawn_robot_nodes)
    ])
