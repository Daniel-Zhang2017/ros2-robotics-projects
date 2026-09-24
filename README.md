# ros2-smart-car
smart car
ROS2+slam+web video server+ttl

### Project 1: ROS2-YOLOvX Real-Time Detection
# Running procedure

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

Note: if you make any changes in the detection_node.py or launch file, it's very important to delete the build and install log by the following code:
```bash
rm -rf build/ultralytics_ros2 install/ultralytics_ros2
```
Then build the functional package:
```bash
colcon build --packages-select ultralytics_ros2
source install/setup.bash
```
