# ROS2 Project Portfolio
## ros2‑robotics‑projects (continuously updated; please star if you're keen on robotics. Each project code is reliable and carefully checked) 
**A collection of ROS2 robotics projects covering simulation, navigation, computer vision, motion control, custom interface development, and Artificial Intelligence for Robotics.**

<a id="table‑of‑contents"></a>
## 📚 Table of Contents
| # | Project | Focus |
|---|---------|-------|
| 0.1 | [Gazebo: Build Your Own Robot](#project‑0.1) | Gazebo simulation, URDF/Xacro modeling, rqt, Rviz2 |
| 0.2 | [ros2_control](#project‑0.2) | Hardware interfaces, controllers, real‑time control |
| 0.3 | [slam_toolbox](#project‑0.3) | SLAM, mapping, localization |
| 0.4 | [Navigation 2](#project‑0.4) | Fine tuning the config parameters: controller_server (DWB Local Controller), planner_server, costmaps, amcl (Localization)|
| 0.5 | [autopatrol_robot](#project‑0.5) | Autonomous patrol, waypoint following, speaker, capture images |
| 1 | [ROS2‑USB_CAM_YOLOvX Real‑Time Detection](#project‑1) | Real‑time YOLO detection, GPU Acceleration & Model Optimization |
| 2 | [ROS2 Person Detection Alert](#project‑2) | Multi‑node alerting |
| 3 | [System Status + QT Display](#project‑3) | Custom interfaces |
| 4 | [Subscribe topics and alert node based on the status](#project‑4) | Custom alert nodes |
| 5 | [cmd_vel & Topic Remapping](#project‑5) | Turtlesim control |
| 6 | [Face Recognition Service](#project‑6) | ROS2 services |
| 7 | [PID Controller for Turtlesim](#project‑7) | ROS2 Action + PID |
| 8 | [Camera Intrinsic Calibration](#project‑8) | Camera calibration |
| 9 | [Training YOLO with Ultralytics](#project‑9) | CV Model training |
| 10 | [ROS2 Text‑to‑Speech (TTS)](#project‑10) | TTS |
| 11 | [Human Skeleton Recognition and Following: bodyreader](#project‑11) | Real-time human skeleton keypoint detection, gesture recognition and mobile robot control |
| 12 | [GMapping, SLAM Toolbox, and Cartographer](#project‑12) | ROS2 2D Mapping Algorithms |
| 13 | [orb_slam2_ros](#project‑13) | orb_slam2_ros: visual mapping |
| 14 | [Controlling a Physical Robotic Arm](#project‑14) | ROS2 + Moveit2 for Roarm |

> [⬆️ Back to Table of Contents](#table‑of‑contents)
# The Project 0 series are basic learning projects

<a id="project‑0.1"></a>
## Project 0.1: Build Your Own Robot
**project0/robot001**
This package contains the robot description implemented with **URDF** and **Xacro** for ROS 2.
URDF (Unified Robot Description Format) uses XML to define robot links, joints, geometry and physical properties. To reduce redundant XML code, we adopt Xacro, a URDF preprocessor supporting variables, mathematical calculations and reusable component macros. At launch time, Xacro files are expanded into standard URDF automatically.

The launch file (`display_robot.launch.py`) loads the robot model via `robot_state_publisher` to publish TF transforms. `joint_state_publisher_gui` provides sliders to adjust joint angles. Run the launch script and open RViz2, add the RobotModel display, set the fixed frame, then visualise the full robot model.

```bash
# cd the project0/robot001 folder
cd project0/robot001
colcon build --packages-select robot_description
source install/setup.bash
ros2 launch robot_description display_robot.launch.py
```

**project0/robot002: Using xacro to build the robot model, create the world field, and show the robot in Gazebo.**

Description: Using xacro to build the robot model, create the world field, and show the robot in Gazebo.

Gazebo doesn't natively use URDF; it uses SDF. When you load a URDF, Gazebo converts it into an SDF model. For this to work, every link must have `<inertial>` and `<collision>` elements. Without valid mass (greater than zero) and inertia, Gazebo will ignore the link or simulate it incorrectly.

Standard URDF tags aren't enough for simulation—you need Gazebo‑specific tags:

- `<gazebo reference="link_name">`: Attaches SDF properties to a link, bridging your URDF with simulation data.
- Material: URDF `<material>` tags (for RViz colors) are ignored by Gazebo. Use `<gazebo reference="link_name"><material>Gazebo/Orange</material></gazebo>` instead.
- Physics: Define friction (`mu1`, `mu2`), contact stiffness (`kp`), and damping (`dampingFactor`) inside `<gazebo>`.
- Plugins: Sensors (Lidar, Camera) and actuators (Differential Drive) use `<plugin>` tags inside a `<gazebo>` block to define movement and sensing.

```
# cd the project0/robot002 folder
cd project0/robot002
colcon build --packages-select robot_description
source install/setup.bash
ros2 launch robot_description gazebo_robot.launch.py
```

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑0.2"></a>
## Project 0.2: ros2_control

## What is ros2_control

`ros2_control` is the standard robot control stack for ROS 2. It decouples robot controllers from low-level hardware drivers, enabling the **same controller code to run in simulation, mock mode, and on physical robots without modification**.

### Core Components

1. **Controller Manager**
The core real-time node (`ros2_control_node`). It runs the control loop: `read hardware → update controllers → write commands`. It manages controller lifecycles (configure / activate / deactivate / cleanup) and allocates command/state interfaces between hardware and controllers via ROS 2 services.
2. **Hardware Interfaces (`hardware_interface`)**
Pluginlib-based hardware abstraction plugins, defined inside URDF with the `<ros2_control>` tag. Four types:

- `SystemInterface`: Multi-joint robot system
- `ActuatorInterface`: Single degree-of-freedom actuator
- `SensorInterface`: Read-only sensor
- `GPIOInterface`: Digital input/output

Hardware plugins implement `read()` (fetch sensor/joint state) and `write()` (send command to motors).

3. **Controller Interfaces (`controller_interface`)**
Controllers are also pluginlib plugins. They consume **state interfaces (read)** and claim **command interfaces (write)**.
Common pre-built controllers from `ros2_controllers`:

- `joint_state_broadcaster`: Publish joint states to `/joint_states`
- `joint_trajectory_controller`: Joint trajectory tracking with FollowJointTrajectory action
- `diff_drive_controller`: Differential drive mobile robot controller
- `forward_command_controller`: Direct passthrough of position/velocity/effort commands

### Workflow

URDF defines hardware resources → YAML config loads controllers → Controller Manager loads plugins → Real-time loop runs controllers → Controllers read joint states and output commands to hardware.

ROS2 Application Layer (Topics/Actions/Services)
↓ ↑
Controller Manager (ros2_control_node, realtime loop)
↓ ↑
Controllers (joint_trajectory_controller, custom controller)
↓ ↑
Hardware Interfaces (System / Actuator / Sensor plugins)
↓ ↑
Robot Hardware / Gazebo Simulation

## What ROS Control is About

Its core can be summed up in one sentence: **read state, compute control, write command**.

The best way to understand `ros_control` is not to memorize package names first, but to remember its main loop:

```
while (running) {
    read();                     // Read hardware states
    controller_manager.update();// Controller computation
    write();                    // Write hardware commands
}
```

These three steps form the heart of the entire framework.

- `read()` fetches states from hardware, such as joint positions, velocities, encoder readings and current feedback.
- `update()` makes controllers compute outputs based on current states and setpoints.
- `write()` sends the computed control signals down to low-level drivers.

You can think of ros_control as a control pipeline:

> 
> Hardware State → Controller Calculation → Control Command → Hardware Execution

Nearly all related concepts revolve around this pipeline.

- `hardware_interface` handles `read()` and `write()`
- `controller` takes charge of `update()`
- URDF and `transmission` tell the system which joints the robot has, and how these joints map to real actuators.
```
# cd the project0/robot003 folder
cd project0/robot003
colcon build --packages-select robot_description
source install/setup.bash
ros2 launch robot_description gazebo_robot.launch.py
```
```bash
# Use the keyboard to control the robot in Gazebo
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```
> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑0.3"></a>
## Project 0.3: slam_toolbox

A robot navigation system mainly consists of **perception and localization, path planning, and motion control**.

1. **Perception and Localization Module**: It acquires environmental information through sensors such as LiDAR and IMU, and calculates the robot’s real-time position and attitude with the pre-built map, answering where the robot is.
2. **Path Planning Module**: It includes global planning and local planning. Global planning generates an optimal route from the start point to the target point based on the static map. Local planning detects dynamic obstacles in real time and adjusts trajectories for obstacle avoidance, determining which path the robot should follow.
3. **Motion Control Module**: It receives the reference trajectory from the planner, computes linear and angular velocity commands, and drives the chassis to track the target path for motion execution, solving how the robot moves.

## SLAM Mapping with slam_toolbox (Online Async Mode)
We use `online_async_launch.py` for real-time 2D LiDAR SLAM.
> Asynchronous mode processes the newest laser scan when computation is busy, suitable for embedded hardware with limited computing resources.

### Prerequisite
- 2D LiDAR publishing `/scan`
- Robot odometry published at `/odom`
- Complete TF tree from laser to robot base

```
# cd the project0/robot003 folder
cd project0/robot003
colcon build --packages-select robot_description
source install/setup.bash
ros2 launch robot_description gazebo_robot.launch.py
```
### Launch command
```bash
ros2 launch slam_toolbox online_async_launch.py use_sim_time:=True
```
New terminal (RViz2):
```
rviz2 --ros-args -p use_sim_time:=True
```
New Terminal(teleop to drive robot):

```
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -p use_sim_time:=True
```
## nav2_map_server
`nav2_map_server` is the official Nav2 map handling package. It provides map loading (`map_server`) and map saving (`map_saver_cli`) utilities for occupancy grid maps used by Nav2 global / local planners.

> - `.posegraph` saved by slam_toolbox: used for slam_toolbox localization (retains loop closure graph)
> - `.yaml + .pgm` saved by nav2_map_server: standard map format for Nav2 navigation.

### Save map from slam_toolbox live /map topic
```bash
ros2 run nav2_map_server map_saver_cli -f my_sim_map
```
> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑0.4"></a>
## Project 0.4: Navigation 2
📌**Overview**

Navigation 2 (Nav2) is the official modern navigation stack for ROS 2. It is a modular, extensible, and production-grade framework for mobile robot autonomous navigation. This repository packages a fully configured Nav2 environment tailored for ROS 2 Humble, supporting simulation testing and real-world robot deployment.

This project includes core Nav2 modules: global path planning, local trajectory control, SLAM mapping, AMCL localization, behavior tree navigation, and RViz visualization tools.

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install ros-humble-navigation2 ros-humble-nav2-bringup

cd project0/robot004
colcon build
source install/setup.bash
ros2 launch robot_description gazebo_robot.launch.py
```
```bash
ros2 launch robot_navigation2 navigation2.launch.py
```

**All Nav2 core parameters are customizable in the config/ folder: project0/robot004/src/robot_navigation2/config/nav2_params.yaml**

# How to tune `nav2_params.yaml` (Nav2 Humble)

> 
> Step-by-step tuning workflow + parameter adjustment rules, ordered from highest priority to fine tuning.

## 1. Pre-tune checklist (must set first)

These are hardware-dependent, wrong values cause immediate failures.

1. **Robot dimension**: `robot_radius` / `footprint`
   - `robot_radius`: actual radius of your robot. `inflation_radius` must be **> robot_radius** (safety buffer).
   - If your robot is rectangular: remove `robot_radius`, define `footprint: [[x1,y1], [x2,y2]...]`
2. **TF & sensor topic**: confirm LiDAR topic name (`/scan`), frame names (`base_link`, `map`, `odom`)
3. **Speed limits**: `max_vel_x`, `max_vel_theta`, `acc_lim_x`. Never set higher than robot hardware limits.

## 2. Tuning order

### Step 1: Costmaps (global + local)

Goal: Robot does not collide, does not get stuck in narrow gaps.

- `inflation_radius`:
  - Too small → collision risk
  - Too large → robot cannot pass narrow doorways
  - Rule: `inflation_radius = robot_radius + safety_margin (0.1~0.2 m)`
- `cost_scaling_factor`: controls how fast obstacle cost fades away
  - Higher: obstacle influence drops sharply; robot can go closer to obstacles
  - Lower: robot keeps further away from obstacles
- `update_frequency`
  - local_costmap: 5–10 Hz for dynamic obstacle
  - global_costmap: 1–2 Hz (no need high frequency)
- `marking: true, clearing: true` → enable adding/removing obstacles from LiDAR

### Step 2: AMCL Localization

Goal: No pose drift, particle cloud converges.

- `particlecloud_min_size / max_size`
  - Start: min=500, max=2000.
  - Localization noisy → increase min particles. (higher = more compute)
  - High CPU load → reduce particle count.
- `update_min_d` / `update_min_a`: minimum movement to trigger AMCL update
  - Too small: frequent updates, high CPU, jitter
  - Too large: slow to correct pose error
  - Typical: `0.2 m`, `0.2 rad`
- `a1,a2,a3,a4` odom noise:
  - Odom drifts easily → increase these four noise values
  - Odom is very good → decrease them
- `laser_max_range`: match your LiDAR spec.

### Step3: DWB Controller (local planner, robot motion)

Goal: Smooth movement, stop near goal, no oscillation.

- `max_vel_x`: start low (0.2~0.3 m/s), gradually increase after stable
- `acc_lim_x`: small value for smoother motion
- `xy_goal_tolerance` / `yaw_goal_tolerance`: goal arrival condition
  - Robot overshoot goal → reduce tolerance
  - Robot stops far away → increase tolerance
- `sim_time`: DWB trajectory preview window
  - Larger sim_time: looks further ahead, slower response
  - Smaller: responsive but easy to oscillate
  - Default ~1.5s works for most differential robots
- Robot wiggling / oscillating:
  - Reduce `max_vel_theta`
  - Lower acceleration
  - Reduce sim_time

### Step4: Global Planner (NavFn)

Goal: global path is reasonable.

- `tolerance`: global path goal tolerance
- `allow_unknown: true`: allow planning into unmapped area; set false if you only want navigation inside mapped region
- `use_astar: true`: A* search (slower but better path), false = Dijkstra

### Step5: Behavior Tree (optional)

- `bt_loop_duration`: BT tick rate, usually 100ms
- If recovery behaviours (spin, backup) trigger too often, go back to tune controller/costmap, not BT first.

## 3. Iterative tuning workflow

1. Set all base hardware params first, start with conservative low speed.
2. Launch Nav2, set 1 simple goal in RViz.
3. Change **only ONE parameter per test**, record result.

> 
> ❌ Never modify multiple parameters in one run; you cannot know which one caused change.

4. Increase speed gradually after robot navigates safely.

## 4. Common symptoms & fix table

| Symptom | Parameter to adjust |
| --- | --- |
| Robot hits obstacle | ↑ inflation_radius, check footprint |
| Cannot go through narrow passage | ↓ inflation_radius |
| Localization drifts / lost | ↑ AMCL particles, tune a1~a4, check TF |
| Robot oscillate/wiggle | ↓ max_vel_theta, ↓ acceleration |
| Overshoot goal | ↓ xy_goal_tolerance / yaw_goal_tolerance |
| Stuck, no path found | ↓ inflation_radius, check allow_unknown |
| High CPU usage | Reduce AMCL particles, lower costmap frequency |

## 5. Useful tips

- After edit yaml: `colcon build --symlink-install` (symlink, no need rebuild every time)
- Use RViz to observe: costmap, particle cloud, planned path, DWB preview trajectory
- Tune in simulation first, then deploy to physical robot. Simulation parameters are not directly copy-paste to real robot.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑0.5"></a>
## Project 0.5: autopatrol_robot

> 
> *TODO: update autopatrol_robot project soon*
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑1"></a>
# Project 1: ROS2‑USB_CAM_YOLOvX Real‑Time Detection

**A ROS 2 package for real‑time object detection using a USB camera and Ultralytics YOLO models.**
**Prerequisites:** Ubuntu 22.04, ROS 2 Humble, Python 3.10

## Running procedure

### 0. Clone the code and install dependencies

```
git clone https://github.com/Daniel‑Zhang2017/ros2‑robotics‑projects.git
mkdir -p ros2_ws/src
# Now move all content from ros2‑robotics‑projects
mv ros2‑robotics‑projects/* ros2_ws/src/
rm -rf ros2‑robotics‑projects
```

Install all main dependencies for `ultralytics_ros2` (ROS2 Humble, Ubuntu 22.04, Python3.10)

```
chmod +x install_deps.sh
./install_deps.sh
```

### 1. Start camera node

```
colcon build --packages-select usb_cam
source install/setup.bash
ros2 launch usb_cam usb_cam_launch.py
```

### 2. New terminal

```
source install/setup.bash
ros2 launch ultralytics_ros2 yolo.launch.py
```

or

```
source install/setup.bash
ros2 run ultralytics_ros2 detection_node
```

### 3. View result via rqt_image_view

```
rqt_image_view
```

Select topic `/detected_image`

> 
> Note: If you modify `detection_node.py` or any launch file, **you must clear the old build, install and log files** with the command below before recompiling.

```
rm -rf build/ultralytics_ros2 install/ultralytics_ros2
```

Then build the functional package:

```
colcon build --packages-select ultralytics_ros2
source install/setup.bash
```

### GPU Acceleration & Model Optimization: `yolo_onnx.launch.py`

**Verify CUDA Availability**

```
python3 - << 'EOF'
import torch
print(f'CUDA available: {torch.cuda.is_available()}')
if torch.cuda.is_available():
    print(f'GPU: {torch.cuda.get_device_name(0)}')
    print(f'VRAM: {torch.cuda.get_device_properties(0).total_memory / 1e9:.1f} GB')
EOF
```

#### Export to ONNX for Faster CPU Inference

For environments without a GPU, exporting the PyTorch model to ONNX can significantly speed up CPU inference (thanks to ONNXRuntime optimizations):

```
from ultralytics import YOLO
model = YOLO('yolov8n.pt')
# Export to ONNX (run once)
model.export(format='onnx', imgsz=640, opset=12)
# Produces yolov8n.onnx
```

Install onnxruntime:

```
uv pip install onnxruntime       # CPU
# or
uv pip install onnxruntime‑gpu   # GPU
```

Run inference with the ONNX model (roughly 1.5–2x speedup).

#### Parameter Tuning & Performance Impact

```
# Speed vs. accuracy trade‑off
# 1) Lower input resolution (most effective speedup)
model.predict(source=img, imgsz=320)   # ~160 fps, but lower small‑object recall
model.predict(source=img, imgsz=640)   # ~80 fps,  standard resolution

# 2) Raise confidence threshold (reduces NMS workload)
model.predict(source=img, conf=0.5)    # Faster, but more missed detections

# 3) Use half precision (GPU only)
model.predict(source=img, half=True)   # FP16, ~2x faster
```

**Launch the ONNX node**

```
colcon build --packages-select ultralytics_ros2
source install/setup.bash
ros2 launch ultralytics_ros2 yolo_onnx.launch.py
```

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑2"></a>
# Project 2: ROS2 Person Detection Alert Project

This project combines USB camera capture, YOLO object detection, and a secondary person recognition node.
The system captures live video stream, runs YOLO inference to draw annotated frames, then a separate node listens to the annotated image topic and triggers an alert message when a human (COCO class `person`) is detected.

## System Architecture

### Nodes:

1. **usb_cam**: USB webcam driver, publishes raw image `sensor_msgs/Image` on `/image_raw`
2. **ultralytics_ros2/detection_node**: YOLO detector. Takes `/image_raw`, runs object detection, draws bounding boxes, publishes annotated image to `/detected_image`
3. **person_detector/person_detector_node**: Subscribes `/detected_image`, performs YOLO inference again. If `person (class id=0)` is found, publish `std_msgs/String` message to `/person_alert`

> 
> ⚠️ Note: This design runs YOLO twice (two separate model loads). This is for demonstration. For better CPU performance, move person alert logic directly into `ultralytics_ros2` node so YOLO runs only once.
> When you use `camera_person_track.launch.py`, please revise `person_detector_node.py` by subscribing the topic of `/image_raw`, instead of `/detected_image`.

**Build the package**

```
cd ~/ros2_ws
# clean old build files
rm -rf build/person_detector install/person_detector
colcon build --packages-select person_detector
source install/setup.bash
```

**Run the full system**

```
ros2 launch person_detector camera_yolo_person.launch.py
# check topic frequency
ros2 topic hz /person_alert
```

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑3"></a>
# Project 3: Publish system status and show in QT page

## Description

Under the `topic_practice_ws/src` folder, there are three subfolders: `status_display`, `status_interfaces`, and `status_publisher`. When you run the code, you can view system status information including `host_name`, `cpu_percent`, `memory_percent`, `memory_total`, `memory_available`, **`battery_percent` (remaining battery level)**, and more.

```
cd topic_practice_ws
colcon build --packages-select status_interfaces
source install/setup.bash
colcon build --packages-select status_display status_publisher
source install/setup.bash
```

```
ros2 run status_publisher sys_status_pub
ros2 run status_display sys_status_display
```

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑4"></a>
# Project 4: sys_status_monitor

ROS2 Humble package for system status monitoring and multi‑level battery alerts.
Subscribes to custom `/sys_status` topic from `status_interfaces`.

## Features

- Console subscriber for system status
- 3‑level battery alert (Warning / Critical / Danger)
- Skip alerts when no battery hardware (`battery_percent=-1.0`)


| Level | Condition | Log Level |
| --- | --- | --- |
| Danger | <10% | FATAL |
| Critical | 10‑19% | ERROR |
| Warning | 20‑39% | WARN |
| Normal | ≥40% | INFO |

## Prerequisites

- ROS2 Humble
- `status_interfaces` (custom `SystemStatus` message)

## Build

```
cd ~/ros2_ws/src
git clone <repo‑url> sys_status_monitor
cd ..
colcon build --packages-select sys_status_monitor
source install/setup.bash
```

## Package Structure

```
sys_status_monitor/
├── sys_status_monitor/
│   ├── __init__.py
│   ├── subscriber_node.py
│   ├── alert_node.py             # set threshold
│   └── battery_bridge.py          # publish battery_status topic
├── launch/sys_status_launch.py   # run nodes together
├── package.xml
└── setup.py
```

## Nodes

```
# Print system status
ros2 run status_publisher sys_status_sub
# Battery alert logic
ros2 run sys_status_monitor battery_alert
```

## Run

Terminal 1 (publisher):

```
ros2 run status_publisher sys_status_pub
```

Terminal 2:

```
ros2 run sys_status_monitor sys_status_sub
```

Terminal3:

```
ros2 run sys_status_monitor battery_alert
```

Launch all nodes:

```
ros2 launch sys_status_monitor sys_status_launch.py
```

## Topics

| Topic | Message Type |
| --- | --- |
| `/sys_status` | `status_interfaces/msg/SystemStatus` |

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑5"></a>
# Project 5: Understanding cmd_vel and topic remapping

## Description

Workspace path: `topic_practice_ws/src`

Write your own publisher node to make the turtle draw a circle automatically — Goal: Understand `cmd_vel` control.
Control the second turtle and practice **topic remapping** — Goal: Master entry‑level skills of multi‑robot control.

This ROS 2 Humble Python node publishes `Twist` messages to `/turtle1/cmd_vel` at 10 Hz to drive the turtlesim turtle along a circular path. When interrupted with `Ctrl+C`, it publishes a zero‑velocity Twist command to stop the turtle before shutting down the node.

```
cd topic_practice_ws
colcon build --packages-select turtle_draw
source install/setup.bash

# Terminal1
ros2 run turtlesim turtlesim_node
# Terminal2
ros2 run turtle_draw draw_circle
```

Spawn a second turtle.

```
ros2 service call /spawn turtlesim/srv/Spawn "{x: 8.0, y: 8.0, theta: 0.0, name: 'turtle2'}"
```

`turtle2` appears in the top‑right corner of the window. At this point, `ros2 topic list` will show an additional set of topics: `/turtle2/cmd_vel` and `/turtle2/pose`.

```
ros2 run turtle_draw draw_circle --ros‑args --remap /turtle1/cmd_vel:=/turtle2/cmd_vel
```

Syntax meaning: `--remap original_name:=new_name`. After startup, the publisher object inside the node remains unchanged, but the actual data flows to `/turtle2/cmd_vel` — and turtle2 in the top‑right corner starts drawing a circle.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑6"></a>
# Project 6: Face Recognition with ROS2 Service

## Description

Package path: `topic_practice_ws/src/demo_python_service`; includes `face_detect_client_node.py` and `face_detect_node.py`.
`learn_detect_from_camera.py` combines `face_recognition` and OpenCV for detecting faces from local webcam.

```
pip3 install face_recognition # or using pip3 install face_recognition -i https://pypi.mirrors.ustc.edu.cn/simple

cd topic_practice_ws
colcon build --packages-select services_interfaces
source install/setup.bash
colcon build --packages-select demo_python_service
source install/setup.bash
```

```
# Terminal1
ros2 run demo_python_service face_detect_client_node
# Terminal2
ros2 run demo_python_service face_detect_node
```

This demo implements real‑time face detection using `face_recognition` and OpenCV. It reads live video stream from the local webcam, detects human faces and draws bounding boxes on each detected face. The code is built for ROS2, and uses `ament_index_python` to access package resource files. Press `q` in the display window to stop the program.

```
ros2 run demo_python_service learn_detect_from_camera
```

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑7"></a>
# Project 7: PID Controller for Turtlesim with ROS2 Action Server & Client

## Description

Workspace path: `topic_practice_ws/src/turtle_demo_controller`.
Custom action definition `GoToPose` located in `cpp_node/action/GoToPose.action`.

The package implements a custom action called `GoToPose` which commands the turtlesim turtle to navigate to a desired `(x, y)` position. The action server computes velocity commands using a PID controller based on the turtle's current pose, while the action client sends goals and listens for feedback and results.

🛠️ Build
From the root of your ROS 2 workspace:

```
colcon build --packages-select turtle_demo_controller
source install/setup.bash
```

Ensure `cpp_node` (which provides the `GoToPose` action) is also built and sourced.

🚀 Run
Terminal 1 — Start turtlesim

```
ros2 run turtlesim turtlesim_node
```

Terminal 2 — Start the action server

```
ros2 run turtle_demo_controller turtle_controller
```

Terminal 3 — Send a goal with the client

```
ros2 run turtle_demo_controller client
```

You'll be prompted:

```
Enter the desired X position: 8.0
Enter the desired Y position: 6.0
```

The turtle will rotate and move toward the target while the client prints feedback.

🧠 **How It Works**

1. The client sends a `GoToPose.Goal` with target `(x, y)`.
2. The server's `goal_callback` validates the request and accepts or rejects it.
3. The server's `execute_callback` subscribes to the turtle's pose and starts a timer.
4. On each pose update:
   - Publishes feedback with current position.
   - Computes `err_dist` and `err_theta`.
   - Applies PID control to produce linear and angular velocities.
   - Publishes `Twist` message to `/turtle1/cmd_vel`.
5. When errors fall within tolerance, the goal is marked as succeeded and result is returned.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑8"></a>
# Project 8: Camera Intrinsic Calibration for ROS2

## Why calibrate the camera

Real camera lenses introduce radial and tangential distortion, bending straight lines and causing pixel position errors. Intrinsic calibration computes the camera intrinsic matrix (focal length, principal point) and distortion coefficients. These parameters are required for SLAM, visual odometry, 3D reconstruction and object pose estimation.

This guide uses official ROS2 `camera_calibration` package with a chessboard pattern to get intrinsic parameters.

## Prerequisites

- ROS2 installed (Humble / Iron / Rolling, replace `<ros2‑distro>` with your distro)
- A printed chessboard calibration plate
- A camera publishing raw image topic

### Install dependencies

```
sudo apt install ros‑<ros2‑distro>‑camera‑calibration
sudo apt install ros‑<ros2‑distro>‑camera‑info‑manager
```

## Step 1: Prepare chessboard pattern

Download printable chessboard:
[https://calib.io/pages/camera‑calibration‑pattern‑generator](https://calib.io/pages/camera%E2%80%91calibration%E2%80%91pattern%E2%80%91generator)

> 
> ⚠️ Important:
> `--size` = **inner corner count**, NOT the number of black/white squares.
> Example: 12×14 squares → inner corners `11x13`.
> `--square` = physical side length of one square (in meters, measure after printing).

## Step 2: Launch calibration node

Open a terminal and run, modify parameters for your chessboard and camera topic:

```
ros2 run camera_calibration cameracalibrator --size 6x9 --square 0.014 image:=/camera/color/image_raw
```

- `--size 6x9`: chessboard inner corners
- `--square 0.014`: square size in meters
- `image:=/camera/color/image_raw`: your raw image topic (may be `/camera/rgb/image_raw`)

## Step 3: Start your camera

New terminal, launch camera driver:

```
ros2 launch turn_on_wheeltec_robot wheeltec_camera.launch.py
# or your own camera ROS2 node
```

## Step 4: Capture calibration samples

A GUI window opens. On the right are four progress bars: `X`, `Y`, `Size`, `Skew`.

1. Move the chessboard in front of camera: left/right, up/down, forward/backward, tilt and rotate.
2. Cover all regions of the image frame with different distances and angles.
3. Stop when **all 4 bars turn green**, and `CALIBRATE` button becomes dark green.

> 
> Lighting should be uniform, avoid glare or overexposure.

## Step 5: Compute and save calibration

1. Click `CALIBRATE`. The GUI freezes; **do not close it**.
2. Wait for terminal output of intrinsic matrix and distortion values.
3. Click `SAVE`.
4. Output archive saved at `/tmp/calibrationdata.tar.gz`.
5. Extract `ost.yaml` inside this archive — this file stores your camera intrinsic and distortion parameters.
6. Click `COMMIT` if you want to register parameters to `camera_info_manager`.

## Step 6: Use calibrated parameters

Load `ost.yaml` with `camera_info_manager` node in your ROS2 vision pipeline to publish corrected camera info.

## Troubleshooting

- `CALIBRATE` grey: insufficient samples. Collect more frames with varied position/tilt.
- No chess corner detection: improve lighting, keep chessboard flat, reduce reflection.
- Topic error: check available topics with `ros2 topic list` and update the `image:=` argument.

## Notes & Common Pitfalls

1. **Print scaling error**: When printing the chessboard, disable "fit to page". If the printer scales the image, your `--square` measurement will be wrong and calibration results invalid. Always physically measure the printed square size with a ruler.
2. **Flat board requirement**: Do not use curved paper. Tape the chessboard flat onto rigid cardboard to avoid bending. Board deformation leads to bad calibration.
3. **Focus & exposure lock**: If your camera supports auto‑focus / auto‑exposure, disable them before calibration. Changing focus during capture alters intrinsic parameters.
4. **Sample distribution**: Do not only hold the chessboard at the image centre. Capture samples in corners, near edges, at close range and far range. This greatly improves robustness.
5. **Reprojection error**: After calibration, check the terminal reprojection error. An error > 1 pixel usually means poor samples or a warped chessboard; redo calibration.
6. **Do not move camera**: Keep the camera fixed during the whole calibration process. Only move the chessboard.
7. **File permission**: `/tmp/` files are temporary and deleted after reboot. Copy `ost.yaml` to your project folder immediately after calibration.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑9"></a>
# Project 9: Training YOLO Models with Ultralytics

🗂 **Dataset Preparation**
Ultralytics expects datasets in YOLO format.

Directory Structure

```
dataset/
├── images/
│   ├── train/
│   └── val/
└── labels/
    ├── train/
    └── val/
```

Each image in `images/` must have a matching `.txt` file in `labels/` with the same filename.

Label Format
Each line in a label file represents one object:

```
<class_id> <x_center> <y_center> <width> <height>
```

All coordinates must be normalized to `[0, 1]`.
`class_id` is a zero‑based integer.

**Example label line**

```
0 0.512 0.634 0.221 0.418
1 0.104 0.882 0.089 0.150
```

Create `data.yaml`

```
path: /absolute/path/to/dataset
train: images/train
val: images/val
names:
  0: person
  1: car
  2: dog
```

🏋️ Training

```
from ultralytics import YOLO
# Load a pretrained model
model = YOLO("yolo26n.pt")  # n / s / m / l / x

# Train
model.train(
    data="data.yaml",
    epochs=100,
    imgsz=640,
    batch=16,
    project="runs/train",
    name="exp1",
    device=0,            # 0 for GPU, "cpu" for CPU
    workers=8,
    pretrained=True,
    optimizer="auto",
    patience=50,
    save=True,
    plots=True,
)
```

CLI

```
yolo detect train \
  model=yolo26n.pt \
  data=data.yaml \
  epochs=100 \
  imgsz=640 \
  batch=16 \
  project=runs/train \
  name=exp1
```

🔁 Resuming Training
If training was interrupted:

```
from ultralytics import YOLO
model = YOLO("runs/train/exp1/weights/last.pt")
model.train(resume=True)
```

Or via CLI:

```
yolo detect train resume model=runs/train/exp1/weights/last.pt
```

✅ Validation

```
from ultralytics import YOLO
model = YOLO("runs/train/exp1/weights/best.pt")
metrics = model.val(data="data.yaml", imgsz=640, batch=16)
print(metrics.box.map)  # mAP50‑95
```

CLI:

```
yolo detect val model=runs/train/exp1/weights/best.pt data=data.yaml
```

🔍 Inference
On images

```
from ultralytics import YOLO
model = YOLO("runs/train/exp1/weights/best.pt")
results = model.predict(source="test.jpg", conf=0.25, save=True)
```

On a folder / video / webcam

```
yolo detect predict model=best.pt source=path/to/folder save=True
yolo detect predict model=best.pt source=video.mp4 save=True
yolo detect predict model=best.pt source=0 show=True
```

📤 Export
**Export a trained model to ONNX, TensorRT, CoreML, etc.**

```
from ultralytics import YOLO
model = YOLO("runs/train/exp1/weights/best.pt")
model.export(format="onnx", dynamic=True, simplify=True)
#Supported formats: onnx, torchscript, engine (TensorRT), coreml, tflite, openvino, pb, saved_model, paddle, ncnn.
```

Project file tree

```
your‑project/
├── 📄 data.yaml                  # Dataset configuration (paths + class names)
├── 🐍 train.py                   # Training script
├── 🐍 val.py                     # Validation / evaluation script
├── 🐍 predict.py                 # Inference script
├── 📄 requirements.txt           # Python dependencies
│
├── 📂 dataset/                   # YOLO‑format dataset
│   ├── 📂 images/                # Input images (train/val/test)
│   └── 📂 labels/                # Corresponding .txt label files
│
└── 📂 runs/                      # Auto‑generated by Ultralytics
    └── 📂 train/
        └── 📂 exp1/              # Experiment output directory
            ├── 📂 weights/
            │   ├── 🧠 best.pt    # Best checkpoint (lowest val loss)
            │   └── 🧠 last.pt    # Last epoch checkpoint (for resume)
            ├── 📊 results.csv    # Per‑epoch metrics log
            ├── 📈 confusion_matrix.png
            └── 📉 results.png
```

💡 **Tips & Troubleshooting**

- Out of memory? Reduce `batch` or `imgsz`, or set `batch=-1` for auto‑batching.
- Slow convergence? Start from a pretrained checkpoint (`yolo26n.pt`) instead of `yolo26n.yaml`.
- Overfitting? Increase augmentation (mosaic, mixup, degrees), add dropout, or freeze layers.
- Class imbalance? Use `cls_pw` loss weight or oversample minority classes.
- Reproducibility? Set `deterministic=True` and `seed=42`.
- Multi‑GPU training: `device=0,1,2,3` or `yolo detect train ... device=0,1`.

View full help:

```
yolo detect train --help
```

Or visit the official docs: [https://docs.ultralytics.com/modes/train/](https://docs.ultralytics.com/modes/train/)

📄 License
This project follows the AGPL‑3.0 License unless otherwise stated.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑10"></a>
# Project 10: ROS2 Text‑to‑Speech (TTS)

## tts_make_ros2 - ROS2 Text‑to‑Speech (TTS)

ROS2 package to convert input text to speech audio files using Iflytek offline TTS SDK. You can adjust volume, pitch, speech speed and other audio parameters, output saved `.wav` audio files locally.

## Overview

This package leverages the Iflytek offline text‑to‑speech SDK to synthesize text into wav audio files.

- Configurable audio parameters: volume, pitch, speaking speed, sample‑rate, number pronunciation mode
- Auto‑save generated audio to local folder with timestamp filename
- Support x86_64 and arm64 architectures
- ROS2 Humble tested

> 
> ⚠️ This depends on Iflytek offline SDK libraries (`libmsc.so`) and offline resource files. You need valid Iflytek APPID for offline TTS service.

## Prerequisites

1. ROS2 Humble installed on your robot / SBC
2. Connect your PC to robot WiFi, SSH into robot terminal
3. Iflytek SDK library file `libmsc.so` matching your target architecture (`x64` / `arm64`)

## Folder Structure

```
tts_make_ros2/
├── audio/                # Output folder for generated .wav audio files
├── config/
│   ├── bin/msc/res/tts/  # Iflytek offline tts resource (common.jet etc.)
│   └── tts_params.yaml   # TTS parameter & APPID configuration
├── launch/
│   └── tts_make.launch.py # Main launch file
├── libs/
│   ├── x64/libmsc.so
│   └── arm64/libmsc.so
└── src/
```

## Installation & Build

### Step 1: Deploy SDK `.so` library

Copy the architecture‑matched `libmsc.so` to system library path:

```
# example for x64 platform
cd topic_practice_ws/src/tts_make_ros2/libs/x64
sudo cp libmsc.so /usr/lib
```

> 
> Use `arm64` folder library for ARM‑based main controller boards.

### Step 2: Compile the ROS2 package

```
cd topic_practice_ws
colcon build --packages-select tts
source install/setup.bash
```

## Usage

### 1. Set text to synthesize

Edit `tts_make.launch.py` to change `tts_text` value to your desired speech text:

```
tts_text = {"tts_text": "Hello, this is ROS2 TTS demo"}
```

### 2. Launch TTS node

```
ros2 launch tts tts_make.launch.py
```

When you see log info `正在合成 开始合成`, synthesis is running.
After completion, audio file will be saved under `tts_make_ros2/audio/`.
File name uses timestamp format like `2022‑06‑01_10:01:33.wav`.

## Configure TTS Parameters

Modify `config/tts_params.yaml`, **no re‑compile needed after editing**:

```
tts_node:
  ros__parameters:
    appid: "your_iflytek_appid"
    rdn: 0          # Number pronunciation mode:0‑number priority;1‑full numeric;2‑full string;3‑string priority
    volume: 50      # Volume [0‑100], higher = louder
    pitch: 50       # Voice pitch [0‑100]
    speed: 50       # Speaking speed [0‑100], higher = faster
    sample_rate: 16000 # Support:16000 / 8000
    voice_name: ""
    text_encoding: ""
```

## Fix error `11212` (Offline resource expired)

Error code `11212` means your Iflytek offline resource is expired. Follow these steps to renew:

1. Go to [Iflytek Open Platform](https://www.xfyun.cn), register account
2. Go to Console → My Applications → Create new application
3. Select capability: **Offline Speech Synthesis(Standard version)**, download SDK matching your platform
4. Replace offline resource file `common.jet` inside `config/bin/msc/res/tts/` with file from new SDK
5. Update `appid` value inside `tts_params.yaml` with your new application APPID

> 
> Each real‑name account can create up to 5 applications; each new app gives 90‑day trial period.
> If SDK file name carries an APPID suffix, use that APPID instead of web console displayed value.

## Launch File Explanation

`tts_make.launch.py` workflow:

1. Locate tts package directory
2. Load yaml config for audio parameters
3. Pass input speech text as ROS parameter `tts_text`
4. Start `tts_node` ROS2 node to trigger text‑to‑audio synthesis

## Troubleshooting

1. **Library missing error**: Confirm `libmsc.so` copied to `/usr/lib` and matches CPU architecture
2. **11212 error**: Renew Iflytek offline SDK resource & APPID
3. No wav output: Check write permission for `audio` folder
4. Build failure: Verify package folder name and `CMakeLists.txt` setup

## Notes

- Generated audio outputs as `.wav` format stored under package `audio` subdirectory
- Parameter changes in `tts_params.yaml` take effect immediately, no `colcon build` required
- This package uses Iflytek closed‑source offline SDK; you need to manage your own SDK license and trial period.

## License

> 
> This ROS2 wrapper code is for demonstration. The underlying Iflytek SDK follows Iflytek's official license terms.
> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑11"></a>
# Project 11: BodyReader: Human Skeleton Gesture Controlled Robot (ROS2)
> 
> ROS2 framework for real-time human skeleton keypoint detection, gesture recognition and mobile robot control. Supports pose control, human following, multi-person orientation and RGB target memory.

## System Workflow

```
Human Skeleton Detection
        ↓
Gesture Definition
        ↓
Gesture Driven Robot Motion
        ↓
Feedback (Audio / Visual)
```

The pipeline extracts human body keypoints, interprets predefined body gestures, sends motion commands to the robot, and returns system status via voice or image feedback.

## Skeleton Keypoints

Detected landmarks: Head, Spine, Shoulder Spine, Mid Spine, Base Spine, Left/Right Shoulder, Elbow, Wrist, Hand, Hip, Knee, Foot.

## Prerequisites

- ROS2 (Humble / Iron)
- Python 3.8+
- OpenCV
- Pose estimation library
- ROS2-compatible mobile robot base

## Quick Start

Clone to your ROS2 workspace `src/` directory:

```
git clone https://github.com/Daniel-Zhang2017/ros2-robotics-projects.git
cd ..
colcon build --packages-select bodyreader
source install/setup.bash
```

### Launch Commands

```
# Pose Control (Pose Control, Multi-person Orientation, RGB memory person fusion)
ros2 launch bodyreader bodyinteraction.launch.py

# Human Skeleton Following (Human Tracking module)
ros2 launch bodyreader bodyfollow.launch.py

# Combined pose control + human following
# Default: pose control mode. Cross arms in front of chest to toggle mode
ros2 launch bodyreader final.launch.py
```

## Key Features

- Real-time human skeleton keypoint extraction
- Custom gesture definition for robot motion control
- Human skeleton tracking mode
- Multi-person orientation control
- RGB visual memory for target person recognition
- Gesture-based mode switching (cross arms to switch)
- Audio and visual feedback

## Project Structure

```
bodyreader/
├── launch/
│   ├── bodyinteraction.launch.py
│   ├── bodyfollow.launch.py
│   └── final.launch.py
├── src/
│   ├── skeleton_detector.py
│   ├── gesture_parser.py
│   ├── robot_driver.py
│   └── feedback_node.py
├── config/
│   └── gesture_params.yaml
└── README.md
```

## How It Works

1. `skeleton_detector`: Reads camera frames and outputs body joint coordinates
2. `gesture_parser`: Analyses joint positions to recognise predefined poses
3. `robot_driver`: Converts valid gestures into ROS2 velocity commands
4. `feedback_node`: Provides voice prompts or annotated image feedback

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑12"></a>
# Project 12: 2D Mapping Algorithms: GMapping, SLAM Toolbox, and Cartographer
# ROS2 2D Mapping \& Map Saving

## Description

 This document collects practical ROS2 launch commands for **2D SLAM map construction** and **map saving**\. It supports three mainstream SLAM algorithms: GMapping, Slam Toolbox, and Cartographer\. The exported map files are fully compatible with the NAV2 navigation stack for robot positioning, re\-localization and autonomous navigation tasks\.

> **Important Note**: Maps saved with the `slam_toolbox:=true` parameter adopt a special compression format, which is mandatory for Slam Toolbox\-based NAV2 re\-localization\. Standard maps cannot be used for Slam Toolbox relocation\.
> 
> 

## 2D SLAM Mapping Commands

### 1\. Mapping with GMapping

A lightweight, classic grid\-based SLAM algorithm, suitable for simple indoor flat scene mapping with low computational resource consumption\.

```bash
ros2 launch slam_gmapping slam_gmapping.launch.py
```

### 2\. Mapping with Slam Toolbox

A modern, robust SLAM solution officially recommended by ROS2\. It supports real\-time mapping, loop closure, and is perfectly integrated with the NAV2 navigation system\.

```bash
ros2 launch wheeltec_slam_toolbox online_async_launch.py
```

### 3\. Mapping with Cartographer

A high\-precision SLAM algorithm developed by Google, featuring excellent loop closure detection and anti\-drift performance, suitable for large\-scale and complex indoor scene mapping\.

```bash
ros2 launch wheeltec_cartographer cartographer.launch.py
```

## Map Saving Commands

### Standard Map Saving \(General Navigation\)

Outputs universal standard map files, applicable for most basic NAV2 navigation scenarios\.

```bash
ros2 launch wheeltec_nav2 save_map.launch.py
```

### Slam Toolbox Special Map Saving \(For NAV2 Re\-localization\)

Saves maps in a dedicated format matching Slam Toolbox\. **Required** if you need to use the NAV2 automatic re\-localization function\.

```bash
ros2 launch wheeltec_nav2 save_map.launch.py slam_toolbox:=true
```

## Post\-Mapping Usage Tips

### 1\. Map File Output

After executing the save map command, two core map files will be generated in the current working directory by default:

- `map.pgm`: Grid map raster image file, records the obstacle and free space information of the environment

- `map.yaml`: Map parameter configuration file, stores map resolution, origin coordinates, obstacle threshold and other core parameters for NAV2 loading

### 2\. Usage Scenario Suggestions

- **GMapping**: Recommended for low\-performance devices and simple small\-space scenarios

- **Slam Toolbox**: Preferred solution for daily ROS2 navigation and re\-localization tasks

- **Cartographer**: Suitable for large venues, long\-distance mapping and high\-precision positioning scenarios

### 3\. Common Precautions

- Do not switch SLAM algorithms arbitrarily during a single mapping task

- For re\-localization tasks, ensure the map file matches the SLAM algorithm used

- It is recommended to rename and classify map files after saving to avoid coverage and confusion
> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑13"></a>
# Project 13: ORB-SLAM2-ROS2: a sparse feature point cloud map system

## Description

This package implements pure‑visual 3D dense point‑cloud mapping, 3D sparse point‑cloud mapping, and converts 3D dense point clouds into 2D occupancy grid maps via OctoMap.

> ⚠️ **Important Pre‑requisites**
> 1. Camera calibration is required before use. Each physical camera has different intrinsic parameters; calibrate your camera to achieve good mapping quality.
> 2. Compilation memory requirement: ~13 GB RAM. Increase swap space before compiling this package, otherwise compilation will fail.
> 3. Currently supported hardware: **RGBD camera & Gemini camera** for dense point‑cloud mapping. Monocular / stereo cameras are not officially adapted; you may try self‑adaptation.

## Launch ORB‑SLAM2‑ROS2

### For Astra RGBD Camera

```bash
ros2 launch orb_slam2_ros orb_slam2_Astra_rgbd_launch.py
ros2 run wheeltec_robot_keyboard wheeltec_keyboard
```

## Published ROS 2 Topics

### ORB‑SLAM2‑ROS2 Output Topics

| Topic name | Description |
| --- | --- |
| `/RGBD/debug_image` | Image with extracted ORB feature points |
| `/RGBD/cloud_points` | Dense 3D point‑cloud output |
| `/RGBD/map_points` | Sparse 3D point‑cloud output |
| `/RGBD/pose` | Visual estimated camera pose |

### OctoMap Output Topics

| Topic name | Description |
| --- | --- |
| `/octomap_point_cloud_centers` | OctoMap voxel point cloud |
| `/occupied_cells_vis_array` | OctoMap voxel model visualization |
| `/projected_map` | Projected 2D occupancy grid map |

## Visualization Examples

1. Dense point cloud output from ORB‑SLAM2‑ROS2
2. Sparse point cloud output from ORB‑SLAM2‑ROS2
3. OctoMap voxel point‑cloud model
4. OctoMap projected 2D grid map

> 
> You may add your screenshot images under an `assets/` folder in repository and reference them here.

## Usage Notes

- Pure visual SLAM is sensitive to lighting, texture and viewing angle. Avoid low‑light environments and texture‑less walls.
- Always run keyboard teleoperation node to move robot slowly through the scene for stable visual tracking.
- If tracking is lost, move robot back to previously mapped textured area to recover tracking.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)

<a id="project‑14"></a>
# Project 14: Controlling a Physical Robotic Arm： ROS2+Moveit2 for Roarm

## Overview

`ros2_arm_ws` is a dedicated ROS2 workspace integrated with multiple functional packages for the motion control, simulation, hardware driving and task planning of the Roarm robotic arm. Each independent package undertakes a specific modular function to support the full physical robotic arm control workflow.

Full project details: [https://github.com/waveshareteam/roarm_ws.git](https://github.com/waveshareteam/roarm_ws.git)

## Workspace Package Structure & Functional Description

The workspace is divided into two core module groups: `roarm_main` (core customized packages for Roarm) and `roarm_else` (extended functional packages).

### 1. roarm_main (Core Custom Packages)

#### 1.1 roarm_description

**Function**: Robotic arm model definition and visualization
Stores URDF (Unified Robot Description Format) files and all robot model configuration resources, supporting 3D simulation, model rendering and visual verification of the Roarm robotic arm in ROS2 environment.

#### 1.2 roarm_driver

**Function**: Physical hardware driver
Provides underlying hardware interface adaptation, responsible for data communication and real‑time control of the physical Roarm robotic arm, realizing the connection between ROS2 software system and actual hardware equipment.

#### 1.3 roarm_moveit

**Function**: MoveIt2 kinematics configuration
Integrates all configuration files and core operation parameters for the MoveIt2 motion planning framework, and completes the kinematic control environment setup for the robotic arm autonomous motion planning.

#### 1.4 roarm_moveit_ikfast_plugins

**Function**: IKFast high‑speed kinematics solver
Encapsulates and implements the IKFast inverse kinematics algorithm plugin, which efficiently optimizes the inverse kinematics calculation speed of the robotic arm, ensuring smooth and real‑time motion response.

#### 1.5 roarm_msgs

**Function**: Custom message definition
Defines exclusive custom message types for the Roarm robotic arm system, realizing standardized data transmission and communication interaction between different functional packages and nodes.

#### 1.6 roarm_moveit_cmd

**Function**: Robotic arm automatic control command
Contains rich control scripts and functional nodes, which can send customized motion instructions to the robotic arm to realize automatic movement and fixed‑task execution.

#### 1.7 roarm_moveit_servo

**Function**: Manual keyboard real‑time control
Supports real‑time servo control of the robotic arm via keyboard operation, providing intuitive and flexible manual debugging and motion control methods.

#### 1.8 roarm_moveit_mtc_demo

**Function**: MoveIt Task Constructor (MTC) demo
Provides practical demo cases based on MTC framework, verifies the ability of MTC to decompose, construct and execute complex robotic arm tasks, and supports secondary development of automated composite tasks.

### 2. roarm_else (Extended Functional Packages)

#### 2.1 moveit_servo

**Function**: Extended keyboard servo control
Extended manual control module, compatible with multi‑scene keyboard operation logic, assists in rapid debugging of robotic arm motion status and parameter verification.

#### 2.2 moveit_task_constructor

**Function**: MTC core planning framework
Integrates the official MoveIt Task Constructor core framework, provides basic task modeling, scheduling and execution logic support for complex robotic arm task planning, and is the underlying dependency for automated composite tasks.

> 
> [⬆️ Back to Table of Contents](#table‑of‑contents)