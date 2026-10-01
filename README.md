# ros2-robotics-projects (continue updating, please star)
## 📚 Table of Contents

| # | Project | Focus |
|---|---------|-------|
| 1 | [ROS2-USB_CAM_YOLOvX Real-Time Detection](#project-1) | Real-time YOLO detection |
| 2 | [ROS2 Person Detection Alert](#project-2) | Multi-node alerting |
| 3 | [System Status + QT Display](#project-3) | Custom interfaces |
| 4 | [cmd_vel & Topic Remapping](#project-4) | Turtlesim control |
| 5 | [Face Recognition Service](#project-5) | ROS2 services |
| 6 | [PID Controller for Turtlesim](#project-6) | ROS2 Action + PID |
| 7 | [Camera Intrinsic Calibration](#project-7) | Camera calibration |
| 8 | [Training YOLO with Ultralytics](#project-8) | CV Model training |

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
**When you use camera_person_track.launch.py, please revise the person_detector_node.py by subscribing the topic of /image_raw, instead of /detected_image.**
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

#check topic frequency
ros2 topic hz /person_alert
```

# Project 3: # Publish system status and show in QT page
## Description: 
Under the `topic_practice_ws/src` folder, there are three subfolders: `status_display`, `status_interfaces`, and `status_publisher`. When you run the code, you can view system status information including `host_name`, `cpu_percent`, `memory_percent`, `memory_total`, `memory_available`, **`battery_percent` (remaining battery level)**, and more.

```bash
cd /topic_practice_ws
colcon build --packages-select status_interfaces
source install/setup.bash
colcon build --packages-select status_display status_publisher
source install/setup.bash
```
```bash
ros2 run status_publisher sys_status_pub 
ros2 run status_display sys_status_display
```

# Project 4: # Understanding the cmd_vel, and remap topic
## Description: topic_practice_ws/src
Write your own publisher node to make the turtle draw a circle automatically — Goal: Understand cmd_vel control.
Control the second turtle and practice **topic remapping (remap)** — Goal: Master the entry-level skills of multi-robot control.
This ROS 2 Humble Python node publishes `Twist` messages to `/turtle1/cmd_vel` at 10 Hz to drive the turtlesim turtle along a circular path. When interrupted with Ctrl+C, it publishes a zero-velocity Twist command to stop the turtle before shutting down the node.
```bash
cd /topic_practice_ws
colcon build --packages-select turtle_draw
source install/setup.bash
# Terminal1
ros2 run turtlesim turtlesim_node
# Terminal2
ros2 run turtle_draw draw_circle
```
Spawn a second turtle.
```bash
ros2 service call /spawn turtlesim/srv/Spawn "{x: 8.0, y: 8.0, theta: 0.0, name: 'turtle2'}"
```
turtle2 appears in the top-right corner of the window. At this point, ros2 topic list will show an additional set of topics: /turtle2/cmd_vel and /turtle2/pose.
```bash
ros2 run turtle_draw draw_circle --ros-args --remap /turtle1/cmd_vel:=/turtle2/cmd_vel
```
Syntax meaning: --remap original_name:=new_name. After startup, the publisher object inside the node remains unchanged, but the actual data flows to /turtle2/cmd_vel — and turtle2 in the top-right corner starts drawing a circle.

# Project 5: # using service for face recognition, Understanding the ROS2 service 
## Description: topic_practice_ws/src/demo_python_service; demo_python_service  includes face_detect_client_node.py and face_detect_node.py; learn_detect_from_camera.py node just combines the `face_recognition` and OpenCV for detecing face from the local webcam.

```bash
pip3 install face_recognition # or using pip3 install face_recognition -i https://pypi.mirrors.ustc.edu.cn/simple

cd /topic_practice_ws
colcon build --packages-select services_interfaces
source install/setup.bash
colcon build --packages-select demo_python_service 
source install/setup.bash
```
```bash
# Terminal1
ros2 run demo_python_service face_detect_client_node
# Terminal2
ros2 run demo_python_service face_detect_node
```
This demo implements real‑time face detection using `face_recognition` and OpenCV. It reads live video stream from the local webcam, detects human faces and draws bounding boxes on each detected face. The code is built for ROS2, and uses `ament_index_python` to access package resource files. Press `q` in the display window to stop the program.

```bash
ros2 run demo_python_service learn_detect_from_camera
```
# Project 6: # ROS2 PID Controller Demo for Turtlesim With Action server and client
## Description: topic_practice_ws/src/turtle_demo_controller + The custom action GoToPose: cpp_node package (see cpp_node/action/GoToPose.action);
The package implements a custom action called GoToPose which asks the turtlesim turtle to navigate to a desired (x, y) position. The action server computes velocity commands using a PID controller based on the turtle's current pose, while the action client sends goals and listens for feedback and results.
🛠️ Build
From the root of your ROS 2 workspace: topic_practice_ws/src

```bash
colcon build --packages-select turtle_demo_controller
source install/setup.bash
```
Ensure cpp_node (which provides the GoToPose action) is also built and sourced.

🚀 Run
Terminal 1 — Start turtlesim
```bash
ros2 run turtlesim turtlesim_node
```
Terminal 2 — Start the action server
```bash
ros2 run turtle_demo_controller turtle_controller
```
Terminal 3 — Send a goal with the client
```bash
ros2 run turtle_demo_controller client
```
You'll be prompted:
text
Enter the desired X position: 8.0
Enter the desired Y position: 6.0
The turtle will rotate and move toward the target while the client prints feedback.
🧠 **How It Works**
The client sends a GoToPose.Goal with target (x, y).
The server's goal_callback validates the request and accepts or rejects it.
The server's execute_callback subscribes to the turtle's pose and starts a timer.
On each pose update, the server:
Publishes feedback with the current position.
Computes err_dist and err_theta.
Applies PID control to produce linear and angular velocities.
Publishes a Twist to /turtle1/cmd_vel.
When the errors fall within tolerance, the goal is marked as succeeded and the result is returned.

# Project 7: # Camera Intrinsic Calibration for ROS2
## Description: ## Why calibrate the camera

Real camera lenses introduce radial and tangential distortion, bending straight lines and causing pixel position errors. Intrinsic calibration computes the camera intrinsic matrix (focal length, principal point) and distortion coefficients. These parameters are required for SLAM, visual odometry, 3D reconstruction and object pose estimation.

This guide uses the official ROS2 `camera_calibration` package with a chessboard pattern to get intrinsic parameters.

## Prerequisites

- ROS2 installed (Humble / Iron / Rolling, replace `<ros2-distro>` with your distro)
- A printed chessboard calibration plate
- A camera publishing raw image topic

### Install dependencies

```
sudo apt install ros-<ros2-distro>-camera-calibration
sudo apt install ros-<ros2-distro>-camera-info-manager
```

## Step 1: Prepare chessboard pattern

Download printable chessboard:
[https://calib.io/pages/camera-calibration-pattern-generator](https://calib.io/pages/camera-calibration-pattern-generator)

> 
> ⚠️ Important:
> `--size` = **inner corner count**, NOT the number of black/white squares.
> Example: 12×14 squares → inner corners `11x13`.
> `--square` = physical side length of one square (in meters, measure after printing).

## Step 2: Launch calibration node

Open a terminal and run, modify parameters for your chessboard and camera topic:

```bash
ros2 run camera_calibration cameracalibrator --size 6x9 --square 0.014 image:=/camera/color/image_raw
```

- `--size 6x9`: chessboard inner corners
- `--square 0.014`: square size in meters
- `image:=/camera/color/image_raw`: your raw image topic (may be `/camera/rgb/image_raw`)

## Step 3: Start your camera

New terminal, launch camera driver:

```
ros2 launch turn_on_wheeltec_robot wheeltec_camera.launch.py
#or you own camera ROS2 node
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
3. **Focus & exposure lock**: If your camera supports auto-focus / auto-exposure, disable them before calibration. Changing focus during capture alters intrinsic parameters.
4. **Sample distribution**: Do not only hold the chessboard at the image centre. Capture samples in corners, near edges, at close range and far range. This greatly improves robustness.
5. **Reprojection error**: After calibration, check the terminal reprojection error. An error > 1 pixel usually means poor samples or a warped chessboard; redo calibration.
6. **Do not move camera**: Keep the camera fixed during the whole calibration process. Only move the chessboard.
7. **File permission**: `/tmp/` files are temporary and deleted after reboot. Copy `ost.yaml` to your project folder immediately after calibration.

# Project 8: # Training YOLO Models with Ultralytics
🗂 **Dataset Preparation**
Ultralytics expects datasets in YOLO format.

Directory Structure
text
dataset/
├── images/
│   ├── train/
│   └── val/
└── labels/
    ├── train/
    └── val/
Each image in images/ must have a matching .txt file in labels/ with the same filename.

Label Format
Each line in a label file represents one object:

text
<class_id> <x_center> <y_center> <width> <height>
All coordinates must be normalized to [0, 1].

class_id is a zero-based integer.

**Example:**

text
0 0.512 0.634 0.221 0.418
1 0.104 0.882 0.089 0.150
Create data.yaml
yaml
path: /absolute/path/to/dataset
train: images/train
val: images/val

names:
  0: person
  1: car
  2: dog
  
🏋️ Training
```bash
from ultralytics import YOLO
```
```bash
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
```bash
yolo detect train \
  model=yolo26n.pt \
  data=data.yaml \
  epochs=100 \
  imgsz=640 \
  batch=16 \
  project=runs/train \
  name=exp1
```
**Key Arguments**

Argument	Description	Default

data	Path to data.yaml	—

epochs	Number of training epochs	100

imgsz	Input image size	640

batch	Batch size (-1 = auto)	16

device	0, 0,1, cpu	auto

optimizer	SGD, Adam, AdamW, auto	auto

lr0	Initial learning rate	0.01

freeze	Freeze first N layers	None

patience	Early stopping patience	50

resume	Resume last training	False
 
🔁 Resuming Training
If training was interrupted:

```bash
from ultralytics import YOLO

model = YOLO("runs/train/exp1/weights/last.pt")
model.train(resume=True)
```
Or via CLI:

```bash
yolo detect train resume model=runs/train/exp1/weights/last.pt
```
✅ Validation
```bash
from ultralytics import YOLO

model = YOLO("runs/train/exp1/weights/best.pt")
metrics = model.val(data="data.yaml", imgsz=640, batch=16)
print(metrics.box.map)  # mAP50-95
```
CLI:

```bash
yolo detect val model=runs/train/exp1/weights/best.pt data=data.yaml
```
🔍 Inference
On images
```bash
from ultralytics import YOLO

model = YOLO("runs/train/exp1/weights/best.pt")
results = model.predict(source="test.jpg", conf=0.25, save=True)
```
On a folder / video / webcam
```bash
yolo detect predict model=best.pt source=path/to/folder save=True
yolo detect predict model=best.pt source=video.mp4 save=True
yolo detect predict model=best.pt source=0 show=True
```
📤 Export
**Export a trained model to ONNX, TensorRT, CoreML, etc.**

```bash
from ultralytics import YOLO

model = YOLO("runs/train/exp1/weights/best.pt")
model.export(format="onnx", dynamic=True, simplify=True)
#Supported formats: onnx, torchscript, engine (TensorRT), coreml, tflite, openvino, pb, saved_model, paddle, ncnn.
```
your-project/
├── 📄 data.yaml                  # Dataset configuration (paths + class names)
├── 🐍 train.py                   # Training script
├── 🐍 val.py                     # Validation / evaluation script
├── 🐍 predict.py                 # Inference script
├── 📄 requirements.txt           # Python dependencies
│
├── 📂 dataset/                   # YOLO-format dataset
│   ├── 📂 images/                # Input images (train/val/test)
│   └── 📂 labels/                # Corresponding .txt label files
│
└── 📂 runs/                      # Auto-generated by Ultralytics
    └── 📂 train/
        └── 📂 exp1/              # Experiment output directory
            ├── 📂 weights/
            │   ├── 🧠 best.pt    # Best checkpoint (lowest val loss)
            │   └── 🧠 last.pt    # Last epoch checkpoint (for resume)
            ├── 📊 results.csv    # Per-epoch metrics log
            ├── 📈 confusion_matrix.png
            └── 📉 results.png
            
💡 **Tips & Troubleshooting**
**Out of memory?** Reduce batch or imgsz, or set batch=-1 for auto-batching.

**Slow convergence?** Start from a pretrained checkpoint (yolo26n.pt) instead of yolo26n.yaml.

**Overfitting?** Increase augmentation (mosaic, mixup, degrees), add dropout, or freeze layers.

**Class imbalance?** Use cls loss weight or oversample minority classes.

**Reproducibility?** Set deterministic=True and seed=42.

**Multi-GPU training**: device=0,1,2,3 or yolo detect train ... device=0,1.

```bash
yolo detect train --help
```
Or visit the official docs: https://docs.ultralytics.com/modes/train/

📄 License
This project follows the AGPL-3.0 License unless otherwise stated.
