import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String
from cv_bridge import CvBridge
from ultralytics import YOLO
import cv2
import numpy as np

class PersonDetectorNode(Node):
    def __init__(self):
        super().__init__('person_detector_node')

        # 配置
        self.model_path = "/home/da/ros2_ws/src/ultralytics_ros2/model/yolo11n.pt"
        self.conf_thresh = 0.5

        self.get_logger().info(f"Loading model: {self.model_path}")
        self.model = YOLO(self.model_path)
        self.bridge = CvBridge()

        # 订阅：detected_image 来自你的YOLO节点
        self.sub_img = self.create_subscription(
            Image,
            '/detected_image',
            self.image_callback,
            10
        )

        # 发布消息：检测到人时发送
        self.pub_alert = self.create_publisher(String, '/person_alert', 10)

        self.get_logger().info("Person detector node started, waiting for /detected_image")

    def image_callback(self, msg: Image):
        try:
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding="bgr8")
        except Exception as e:
            self.get_logger().warn(f"Image convert error: {e}")
            return

        # YOLO推理
        results = self.model(cv_image, conf=self.conf_thresh, verbose=False)
        person_found = False

        for res in results:
            boxes = res.boxes
            for box in boxes:
                cls_id = int(box.cls[0])
                # YOLO COCO: class 0 = person
                if cls_id == 0:
                    person_found = True
                    break
            if person_found:
                break

        if person_found:
            alert_msg = String()
            alert_msg.data = "Person detected!"
            self.pub_alert.publish(alert_msg)
            self.get_logger().info("✅ Person detected! Published alert message.")
        else:
            # 可选：不输出日志，减少刷屏
            self.get_logger().info("✅ it's safe. No person!! start UV disinfection.")
            pass


def main(args=None):
    rclpy.init(args=args)
    node = PersonDetectorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
