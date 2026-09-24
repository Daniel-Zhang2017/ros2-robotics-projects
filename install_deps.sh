#!/bin/bash
set -e

echo "=== Update apt packages ==="
sudo apt update && sudo apt upgrade -y

echo "=== Install ROS2 vision_msgs package ==="
sudo apt install -y ros-humble-vision-msgs ros-humble-rclpy ros-humble-sensor-msgs ros-humble-std-msgs

echo "=== Install python pip and required python packages ==="
sudo apt install -y python3-pip python3-venv

echo "=== Install ultralytics YOLO library (user pip, NO sudo) ==="
pip3 install --user ultralytics torch torchvision opencv-python

echo "=== Verify python import test ==="
python3 -c "
try:
    import ultralytics
    from ultralytics import YOLO
    print('[OK] ultralytics imported successfully')
except Exception as e:
    print('[ERROR] ultralytics import failed:', e)
    exit(1)
"

echo "=== Verify ROS2 vision_msgs import test ==="
python3 -c "
try:
    from vision_msgs.msg import Detection2D, Detection2DArray
    print('[OK] vision_msgs imported successfully')
except Exception as e:
    print('[ERROR] vision_msgs import failed:', e)
    exit(1)
"

echo ""
echo "✅ All dependencies installed."
echo "👉 Next step: go to your ros2_ws and rebuild package:"
echo "cd ~/ros2_ws"
echo "colcon build --packages-select ultralytics_ros2"
echo "source install/setup.bash"
echo "ros2 launch ultralytics_ros2 yolo.launch.py"
