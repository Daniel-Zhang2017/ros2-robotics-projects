# ros2-smart-car
smart car
ROS2+slam+web video server+ttl

### ROS2-YOLOvX Real-Time Detection###
# testing procedure

1. ### start camera node

```bash
ros2 launch turn_on_wheeltec_robot wheeltec_camera.launch.py
```

2. ### new terminates

```bash
source install/setup.bash
ros2 launch ultralytics_ros2 yolo.launch.py
```

3. ### rqt_image_view view `/detected_image`
