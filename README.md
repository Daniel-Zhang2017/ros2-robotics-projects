# ros2-robotics-projects (continue updating, please star)

# ROS2+slam+deep learning

# Project 1: ROS2-USB_CAM_YOLOvX Real-Time Detection
## Running procedure
0. ### clone the code and install dependcies
 ```bash
git clone https://github.com/Daniel-Zhang2017/ros2-robotics-projects.git
mkdir -p ros2_ws/src
# Now move all content from ros2‑robotics‑projects
mv ros2-robotics-projects/* ros2_ws/src/
rm -rf ros2-robotics-projects
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

# Project 2: # ROS2 Person Detection Alert Project
This project combines USB camera capture, YOLO object detection, and a secondary person recognition node.
The system captures live video stream, runs YOLO inference to draw annotated frames, then a separate node listens to the annotated image topic and triggers an alert message when a human (COCO class `person`) is detected.

## System Architecture
### Nodes:
1. **usb_cam**: USB webcam driver, publishes raw image `sensor_msgs/Image` on `/image_raw`
2. **ultralytics_ros2/detection_node**: YOLO detector. Takes `/image_raw`, runs object detection, draws bounding boxes, publishes annotated image to `/detected_image`
3. **person_detector/person_detector_node**: Subscribes `/detected_image`, performs YOLO inference again. If `person (class id=0)` is found, publish `std_msgs/String` message to `/person_alert`

> ⚠️ Note: This design runs YOLO twice (two separate model loads). This is for demonstration. For better CPU performance, move person alert logic directly into `ultralytics_ros2` node so YOLO runs only once.
## Build the package

```
cd ~/ros2_ws
# clean old build files
rm -rf build/person_detector install/person_detector
colcon build --packages-select person_detector
source install/setup.bash
```

## Run the full system

```
ros2 launch person_detector camera_yolo_person.launch.py
```
