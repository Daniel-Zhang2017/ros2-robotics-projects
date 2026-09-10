import os
import yaml
from pathlib import Path
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch.actions import GroupAction, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import ComposableNodeContainer

from launch_ros.descriptions import ComposableNode


# -------------------------------------------------
# 工具函数：读取 yaml
# -------------------------------------------------
def load_yaml(file_path: str) -> dict:
    return yaml.safe_load(Path(file_path).read_text())


# -------------------------------------------------
# 根据 imu_mode 生成节点列表
# -------------------------------------------------
def include_imu_launch(context, *args, **kwargs):
    # 1) 读取默认 imu_mode（yaml 中）
    cfg_path = LaunchConfiguration('imu_mode_yaml').perform(context)
    cfg = load_yaml(cfg_path)
    imu_mode = LaunchConfiguration('imu_mode').perform(context) or cfg['imu_mode']
    print(f'imu_mode:{imu_mode}')

    #ultrasonic_avoid = LaunchConfiguration('imu_mode').perform(context) or cfg['ultrasonic_avoid']
    

    parm_file = LaunchConfiguration('wheeltec_param_yaml').perform(context)
    car_cfg = load_yaml(parm_file)
    car_mode = LaunchConfiguration('car_mode').perform(context) or car_cfg['car_mode']

    ultrasonic_avoid =  car_cfg['ultrasonic_avoid']
    print(f'ultrasonic_avoid:{ultrasonic_avoid}')

    # 公共参数
    common_params = {
        'usart_port_name': '/dev/wheeltec_controller',
        'serial_baud_rate': 115200,
        'robot_frame_id': 'base_footprint',
        'odom_frame_id': 'odom_combined',
        'cmd_vel': 'cmd_vel',
        'akm_cmd_vel': 'none',
        'product_number': 0,
        'odom_x_scale': 1.0,
        'odom_y_scale': 1.0,
        'odom_z_scale_positive': 1.0,
        'odom_z_scale_negative': 1.0,
        'car_mode': car_mode,
        'ultrasonic_avoid': ultrasonic_avoid
    }

    nodes = []
    remappings=[('imu/data_raw', 'imu/data_board')]
    # wheeltec 主节点

    if ultrasonic_avoid:
        nodes.append(
                    IncludeLaunchDescription(
                        PythonLaunchDescriptionSource([
                            os.path.join(
                                get_package_share_directory('wheeltec_ultrasonic'),
                                'launch',
                                'ultrasonic_pose.launch.py'
                            )
                        ])
                    )
            )
        
    if imu_mode == 'stm32':
        remappings = []
        
    wheeltec_node = Node(
        package='turn_on_wheeltec_robot',
        executable='wheeltec_robot_node',
        output='screen',
        parameters=[common_params],
        remappings=remappings,
    )
    nodes.append(wheeltec_node)
    if imu_mode == 'stm32':
        remappings = []
    elif imu_mode == 'H30':
        nodes.append(
                    IncludeLaunchDescription(
                        PythonLaunchDescriptionSource([
                            os.path.join(
                                get_package_share_directory('yesense_std_ros2'),
                                'launch',
                                'yesense_node.launch.py'
                            )
                        ])
                    )
            )
    elif imu_mode == 'N300pro':
        nodes.append(
                    IncludeLaunchDescription(
                        PythonLaunchDescriptionSource([
                            os.path.join(
                                get_package_share_directory('hipnuc_imu'),
                                'launch',
                                'imu_spec_msg.launch.py'
                            )
                        ])
                    )
            )

    else:
        raise ValueError(f'Unsupported imu_mode: {imu_mode}')
    return nodes


# -------------------------------------------------
# 主入口
# -------------------------------------------------
def generate_launch_description():

    # 2) 声明可覆盖的参数
    declare_imu_path = DeclareLaunchArgument(
        'imu_mode_yaml',
        default_value=os.path.join(
        get_package_share_directory('turn_on_wheeltec_robot'),
        'config', 'wheeltec_param.yaml'
        ),
        description='Path to imu_mode.yaml'
    )
    declare_imu_mode = DeclareLaunchArgument(
        'imu_mode',
        default_value='',
        description='stm32 or H30 or N300pro'
    )

    declare_carparam_mode = DeclareLaunchArgument(
        'wheeltec_param_yaml',
        default_value=os.path.join(
            get_package_share_directory('turn_on_wheeltec_robot'),
            'config', 'wheeltec_param.yaml'),
        description='Path to wheeltec_param.yaml'
    )
    # 添加 car_mode 的声明
    declare_car_mode = DeclareLaunchArgument(
        'car_mode',
        default_value='',
        description='Car mode configuration'
    )
    
    imu_bias_remover_node = ComposableNode(
    package='imu_processors',
    plugin='imu_processors::ImuBiasRemover',
    name='imu_bias_remover',
    parameters=[{
        'use_odom': True,
        'odom_threshold': 0.005,
        'accumulator_alpha': 0.001
    }],
    remappings=[
        ('imu', '/imu/data_raw'),
        ('cmd_vel', '/cmd_vel'),
        ('imu_biased', '/imu/data_biased')
    ]
    )
    imu_container = ComposableNodeContainer(
    name='imu_container',
    namespace='',
    package='rclcpp_components',
    executable='component_container_mt',
    composable_node_descriptions=[
        imu_bias_remover_node
    ],
    output='screen',)


    # 3) 使用 OpaqueFunction 在运行时生成节点
    return LaunchDescription([
        declare_imu_path,
        declare_imu_mode,
        declare_carparam_mode,
        declare_car_mode,
        #imu_container,
        OpaqueFunction(function=include_imu_launch)
    ])
