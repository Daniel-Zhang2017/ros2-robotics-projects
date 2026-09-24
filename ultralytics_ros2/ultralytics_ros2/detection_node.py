import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from vision_msgs.msg import Detection2D, Detection2DArray, BoundingBox2D, ObjectHypothesisWithPose
from ultralytics import YOLO
import cv2
import numpy as np
import os
import torch
from threading import Lock, Thread

class YOLODetector(Node):
    def __init__(self):
        super().__init__('yolo_detector')
        self.get_logger().info("==== Node init finished, code loaded successfully ====")

        self.declare_parameters(
            namespace='',
            parameters=[
                ('model', 'yolov8s.pt'),
                ('input_image_topic', '/image_raw'),
                ('enable_cuda', True),
                ('conf_threshold', 0.5),
                ('infer_period', 0.08)
            ]
        )
        self.model_name = self.get_parameter('model').value
        self.input_topic = self.get_parameter('input_image_topic').value
        self.enable_cuda = self.get_parameter('enable_cuda').value
        self.conf_threshold = self.get_parameter('conf_threshold').value
        self.infer_period = self.get_parameter('infer_period').value

        self.model_full_path = self.model_name
        self.model = None
        self.bridge = CvBridge()

        self.latest_image = None
        self.latest_msg_header = None
        self.lock = Lock()

        self.sub = self.create_subscription(
            Image, self.input_topic, self.image_callback,
            qos_profile=rclpy.qos.QoSProfile(depth=1)
        )
        self.pub_image = self.create_publisher(Image, 'detected_image', 10)
        self.pub_detections = self.create_publisher(Detection2DArray, 'detections', 10)

        self.infer_timer = self.create_timer(self.infer_period, self.infer_timer_callback)
        self.last_time = self.get_clock().now()

        # Start model loading in background thread, do NOT block rclpy executor
        self.load_thread = Thread(target=self._load_model_worker, daemon=True)
        self.load_thread.start()

    def _load_model_worker(self):
        """Run model loading on separate background thread"""
        self.get_logger().info(f"Start loading model: {self.model_full_path}")
        try:
            self.model = YOLO(self.model_full_path)
            if self.enable_cuda and torch.cuda.is_available():
                self.model.to('cuda')
                self.get_logger().info("Using CUDA GPU")
            else:
                self.get_logger().info("CUDA not available, run on CPU")
            self.model.fuse()
            self.get_logger().info("✅ Model loaded successfully")
        except Exception as e:
            self.get_logger().error(f"Failed to load model: {str(e)}")

    def image_callback(self, msg):
        try:
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
            with self.lock:
                self.latest_image = cv_image
                self.latest_msg_header = msg.header
        except Exception as e:
            self.get_logger().warn(f"callback convert error: {str(e)}")

    def infer_timer_callback(self):
        if self.model is None:
            return

        with self.lock:
            if self.latest_image is None:
                return
            img = self.latest_image.copy()
            header = self.latest_msg_header

        try:
            results = self.model.predict(
                source=img,
                conf=self.conf_threshold,
                verbose=False,
                imgsz=640,
                half=False,
                device=0 if torch.cuda.is_available() else 'cpu'
            )
            result = results[0]
            detections_msg = Detection2DArray()
            detections_msg.header = header
            annotated_image = result.plot()
            annotated_image = np.array(annotated_image, dtype=np.uint8).copy()
            annotated_image = cv2.cvtColor(annotated_image, cv2.COLOR_RGB2BGR)

            current_time = self.get_clock().now()
            delta_time = current_time - self.last_time
            fps = 1e9 / delta_time.nanoseconds if delta_time.nanoseconds > 0 else 0.0
            cv2.putText(annotated_image, f'FPS: {fps:.2f}', (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            img_msg = Image()
            img_msg.header = header
            img_msg.height = annotated_image.shape[0]
            img_msg.width = annotated_image.shape[1]
            img_msg.encoding = "bgr8"
            img_msg.is_bigendian = 0
            img_msg.step = annotated_image.shape[1] * 3
            img_msg.data = annotated_image.tobytes()
            self.pub_image.publish(img_msg)

            for box in result.boxes:
                detection = Detection2D()
                detection.bbox = BoundingBox2D()
                xc, yc, w, h = box.xywh.cpu().numpy()[0]
                detection.bbox.center.position.x = float(xc)
                detection.bbox.center.position.y = float(yc)
                detection.bbox.size_x = float(w)
                detection.bbox.size_y = float(h)
                cls_idx = int(box.cls)
                conf_score = float(box.conf)
                hypothesis = ObjectHypothesisWithPose()
                hypothesis.hypothesis.class_id = str(cls_idx)
                hypothesis.hypothesis.score = conf_score
                detection.results.append(hypothesis)
                detections_msg.detections.append(detection)

            self.pub_detections.publish(detections_msg)
            self.last_time = current_time

        except Exception as e:
            self.get_logger().error(f"infer error: {str(e)}")

def main(args=None):
    rclpy.init(args=args)
    detector = YOLODetector()
    rclpy.spin(detector)
    detector.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
