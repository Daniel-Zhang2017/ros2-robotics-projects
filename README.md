# ros2-smart-car
smart car
ROS2+slam+web video server+ttl

### Project 1: ROS2-YOLOvX Real-Time Detection
# Running procedure
0. clone the code and install dependcies
 ```bash
git clone https://github.com/Daniel-Zhang2017/ros2-robotics-projects.git
```  
install all main dependencies for ultralytics_ros2 (ROS2 Humble, Ubuntu 22.04, Python3.10)
```bash
chmod +x install_deps.sh
./install_deps.sh
```

1. ### start camera node

```bash
ros2 launch usb_cam usb_cam_launch.py
```

2. ### new terminal

```bash
source install/setup.bash
ros2 launch ultralytics_ros2 yolo.launch.py
```
or
```bash
source install/setup.bash
ros2 run ultralytics_ros2 detection_node
```
3. ### rqt_image_view view `/detected_image`

Note: If you modify `detection_node.py` or any launch file, **you must clear the old build, install and log files** with the command below before recompiling.
```bash
rm -rf build/ultralytics_ros2 install/ultralytics_ros2
```
Then build the functional package:
```bash
colcon build --packages-select ultralytics_ros2
source install/setup.bash
```
