import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from vision_msgs.msg import Detection2D, Detection2DArray, BoundingBox2D, ObjectHypothesisWithPose
from ultralytics import YOLO
import cv2
import numpy as np

class YOLODetector(Node):
    def __init__(self):
        super().__init__('yolo_detector')
        
        # 参数声明
        self.declare_parameters(
            namespace='',
            parameters=[
                ('model', 'yolov8n.pt'),
                ('input_image_topic', '/camera/image_raw'),
                ('enable_cuda', True),
                ('conf_threshold', 0.5)
            ]
        )
        
        # 参数获取
        model_path = self.get_parameter('model').value
        input_topic = self.get_parameter('input_image_topic').value
        enable_cuda = self.get_parameter('enable_cuda').value
        self.conf_threshold = self.get_parameter('conf_threshold').value

        # 初始化YOLO模型
        self.model = YOLO(model_path)
        if enable_cuda:
            self.model.to('cuda')
        self.model.fuse()

        # 图像处理工具
        self.bridge = CvBridge()
        
        # 订阅/发布
        self.sub = self.create_subscription(Image, input_topic, self.image_callback, 10)
        self.pub_image = self.create_publisher(Image, 'detected_image', 10)
        self.pub_detections = self.create_publisher(Detection2DArray, 'detections', 10)
        
        # 帧率计算
        self.frame_count = 0
        self.last_time = self.get_clock().now()

    def image_callback(self, msg):
        # 转换图像格式
        cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        
        # 执行推理
        results = self.model.predict(
            source=cv_image,
            conf=self.conf_threshold,
            verbose=False
        )

        # 准备检测结果消息
        detections_msg = Detection2DArray()
        detections_msg.header = msg.header
        
        # 绘制检测结果
        annotated_image = results[0].plot()
        
        # 计算并显示帧率
        current_time = self.get_clock().now()
        delta_time = current_time - self.last_time
        fps = 1e9 / delta_time.nanoseconds if delta_time.nanoseconds > 0 else 0.0
        cv2.putText(annotated_image, f'FPS: {fps:.2f}', (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        # 发布带检测结果的图像
        self.pub_image.publish(self.bridge.cv2_to_imgmsg(annotated_image, 'bgr8'))
        
        # 填充检测结果
        for box in results[0].boxes:
            detection = Detection2D()
            detection.bbox = BoundingBox2D()
            detection.bbox.center.position.x = float(box.xywh[0][0])
            detection.bbox.center.position.y = float(box.xywh[0][1])
            detection.bbox.size_x = float(box.xywh[0][2])
            detection.bbox.size_y = float(box.xywh[0][3])
            
            hypothesis = ObjectHypothesisWithPose()
            hypothesis.hypothesis.class_id = self.model.names[int(box.cls)]
            hypothesis.hypothesis.score = float(box.conf)
            detection.results.append(hypothesis)
            
            detections_msg.detections.append(detection)
        
        # 发布检测结果
        self.pub_detections.publish(detections_msg)
        self.last_time = current_time

def main(args=None):
    rclpy.init(args=args)
    detector = YOLODetector()
    rclpy.spin(detector)
    detector.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()