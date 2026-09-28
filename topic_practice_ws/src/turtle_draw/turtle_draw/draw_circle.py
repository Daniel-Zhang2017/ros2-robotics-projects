#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


class DrawCircle(Node):
    def __init__(self):
        super().__init__('draw_circle')
        # 向 /turtle1/cmd_vel 发布 Twist，队列深度 10
        self.publisher_ = self.create_publisher(Twist, '/turtle1/cmd_vel', 10)
        # 每 0.1 秒（10 Hz）执行一次 timer_callback
        self.timer = self.create_timer(0.1, self.timer_callback)

    def timer_callback(self):
        msg = Twist()
        msg.linear.x = 2.0    # 线速度：向前 2 m/s
        msg.angular.z = 1.0   # 角速度：左转 1 rad/s
        self.publisher_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = DrawCircle()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.publisher_.publish(Twist())  # 退出前发全零速度，让海龟停下
        node.destroy_node()
        rclpy.shutdown()
