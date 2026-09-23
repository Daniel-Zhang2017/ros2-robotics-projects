import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from vision_msgs.msg import (
    Detection2D,
    Detection2DArray,
    BoundingBox2D,
    ObjectHypothesisWithPose,
)
from ultralytics import YOLO
import cv2
import numpy as np
import os
import torch
from threading import Lock


class YOLODetector(Node):
    def __init__(self):
        super().__init__('yolo_detector')
        self.get_logger().info("==== Node init started ====")

        self.declare_parameters(
            namespace='',
            parameters=[
                ('model', 'yolov8s-pose.pt'),
                ('input_image_topic', '/image_raw'),
                ('enable_cuda', True),
                ('conf_threshold', 0.5),
                ('infer_period', 0.08),   # inference period (s) ~ 12.5 Hz
                ('imgsz', 640),
                ('half', False),
            ]
        )

        # --- Parameters -----------------------------------------------------
        model_path = self.get_parameter('model').value
        input_topic = self.get_parameter('input_image_topic').value
        enable_cuda = self.get_parameter('enable_cuda').value
        self.conf_threshold = float(self.get_parameter('conf_threshold').value)
        infer_period = float(self.get_parameter('infer_period').value)
        self.imgsz = int(self.get_parameter('imgsz').value)
        self.half = bool(self.get_parameter('half').value)

        # --- Model ----------------------------------------------------------
        # Prefer the parameter path; fall back to the default location if the
        # parameter does not point to an existing file.
        model_full_path = os.path.expanduser(model_path)
        if not os.path.isfile(model_full_path):
            fallback = os.path.expanduser(
                "~/wheeltec_ros2/src/ultralytics_ros2/model/yolov8s-pose.pt"
            )
            self.get_logger().warn(
                f"Model '{model_full_path}' not found, falling back to '{fallback}'"
            )
            model_full_path = fallback

        self.model = YOLO(model_full_path)

        self.use_cuda = bool(enable_cuda and torch.cuda.is_available())
        if self.use_cuda:
            self.model.to('cuda')
            self.get_logger().info("Using CUDA GPU")
        else:
            self.get_logger().info("CUDA not available or disabled, running on CPU")

        # `fuse()` only makes sense after the model has been moved to its
        # final device. Wrap in try/except because some Ultralytics versions
        # raise if called twice.
        try:
            self.model.fuse()
        except Exception as e:
            self.get_logger().warn(f"model.fuse() skipped: {e}")

        # Ensure the internal YOLO device string matches what we will pass
        # to `predict()`.
        self.device = 0 if self.use_cuda else 'cpu'

        self.bridge = CvBridge()

        # --- Shared state ---------------------------------------------------
        self.latest_image = None
        self.latest_msg_header = None
        self.lock = Lock()

        # --- QoS: keep only the most recent frame ---------------------------
        image_qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
        )

        self.sub = self.create_subscription(
            Image, input_topic, self.image_callback, image_qos
        )

        self.pub_image = self.create_publisher(Image, 'detected_image', 10)
        self.pub_detections = self.create_publisher(Detection2DArray, 'detections', 10)

        # --- Timer-driven inference ----------------------------------------
        self.infer_timer = self.create_timer(infer_period, self.infer_timer_callback)
        self.last_time = self.get_clock().now()

        self.get_logger().info("==== Node init finished, code loaded successfully ====")

    # ------------------------------------------------------------------ #
    def image_callback(self, msg: Image):
        """Store only the newest frame; do no heavy work here."""
        try:
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
            with self.lock:
                self.latest_image = cv_image
                self.latest_msg_header = msg.header
        except Exception as e:
            self.get_logger().warn(f"image_callback convert error: {e}")

    # ------------------------------------------------------------------ #
    def infer_timer_callback(self):
        """Run inference on the most recent frame only."""
        with self.lock:
            if self.latest_image is None:
                return
            img = self.latest_image
            header = self.latest_msg_header
            # Take a reference, not a copy: the next callback will replace
            # the pointer, so this buffer stays valid for our own use.
            self.latest_image = None  # mark as consumed

        try:
            results = self.model.predict(
                source=img,
                conf=self.conf_threshold,
                verbose=False,
                imgsz=self.imgsz,
                half=self.half and self.use_cuda,  # fp16 only on CUDA
                device=self.device,
            )
            result = results[0]

            # --- Annotated image ------------------------------------------
            annotated_image = result.plot()  # BGR already (Ultralytics)
            annotated_image = np.ascontiguousarray(annotated_image)

            current_time = self.get_clock().now()
            delta = current_time - self.last_time
            fps = 1e9 / delta.nanoseconds if delta.nanoseconds > 0 else 0.0
            self.last_time = current_time

            cv2.putText(
                annotated_image,
                f'FPS: {fps:.2f}',
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 0),
                2,
            )

            img_msg = self.bridge.cv2_to_imgmsg(annotated_image, encoding='bgr8')
            img_msg.header = header
            self.pub_image.publish(img_msg)

            # --- Detections ------------------------------------------------
            detections_msg = Detection2DArray()
            detections_msg.header = header

            boxes = result.boxes
            if boxes is not None and len(boxes) > 0:
                xywh = boxes.xywh.cpu().numpy()
                clss = boxes.cls.cpu().numpy().astype(int)
                confs = boxes.conf.cpu().numpy()

                for (xc, yc, w, h), cls_idx, conf_score in zip(xywh, clss, confs):
                    detection = Detection2D()
                    detection.bbox = BoundingBox2D()
                    detection.bbox.center.position.x = float(xc)
                    detection.bbox.center.position.y = float(yc)
                    detection.bbox.size_x = float(w)
                    detection.bbox.size_y = float(h)

                    hypothesis = ObjectHypothesisWithPose()
                    hypothesis.hypothesis.class_id = str(cls_idx)
                    hypothesis.hypothesis.score = float(conf_score)
                    detection.results.append(hypothesis)

                    detections_msg.detections.append(detection)

            self.pub_detections.publish(detections_msg)

        except Exception as e:
            self.get_logger().error(f"infer_timer_callback error: {e}")


# ---------------------------------------------------------------------- #
def main(args=None):
    rclpy.init(args=args)
    detector = YOLODetector()
    try:
        rclpy.spin(detector)
    except KeyboardInterrupt:
        pass
    finally:
        detector.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
