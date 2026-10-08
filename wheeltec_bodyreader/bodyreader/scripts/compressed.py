#!/usr/bin/env python3
# coding=utf-8

import rclpy
from rclpy.node import Node
import cv2
import numpy as np
from sensor_msgs.msg import CompressedImage
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

class ImageConverter(Node):
    def __init__(self):
        super().__init__('compressed')
        self.bridge = CvBridge()
        self.declare_parameter('input_image_topic', '/body/body_display') 
        input_image_topic= self.get_parameter('input_image_topic').get_parameter_value().string_value
        self.declare_parameter('output_image_topic', '/repub/body/body_display/compressed') 
        output_image_topic= self.get_parameter('output_image_topic').get_parameter_value().string_value
        self.image_sub = self.create_subscription(Image, input_image_topic, self.callback, 10)
        self.image_pub = self.create_publisher(CompressedImage, output_image_topic, 1)

    def callback(self, msg):
        image_np = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
        jpeg_img = CompressedImage()
        jpeg_img.header.stamp = self.get_clock().now().to_msg()
        jpeg_img.format = "jpeg"
        #_, img_encoded = cv2.imencode('.jpg', image_np,encode_param)
        jpeg_img.data = np.array(cv2.imencode('.jpg', image_np,encode_param)[1]).tobytes()
        self.image_pub.publish(jpeg_img)


       

if __name__ == '__main__':
    rclpy.init()
    node = ImageConverter()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()
        print('exception')
