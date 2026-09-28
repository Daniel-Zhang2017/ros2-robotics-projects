import rclpy
from rclpy.node import Node
from status_interfaces.msg import SystemStatus  # import custom message interface
import psutil
import platform
import os

class SysStatusPub(Node):
    def __init__(self, node_name):
        super().__init__(node_name)
        self.status_publisher_ = self.create_publisher(
            SystemStatus, 'sys_status', 10)
        self.timer = self.create_timer(1, self.timer_callback)
        # Battery path
        self.battery_path = "/sys/class/power_supply/BAT0/capacity"

    def timer_callback(self):
        cpu_percent = psutil.cpu_percent()
        memory_info = psutil.virtual_memory()
        net_io_counters = psutil.net_io_counters()

        msg = SystemStatus()
        msg.stamp = self.get_clock().now().to_msg()
        msg.host_name = platform.node()
        msg.cpu_percent = cpu_percent
        msg.memory_percent = memory_info.percent
        msg.memory_total = memory_info.total / 1024 / 1024
        msg.memory_available = memory_info.available / 1024 / 1024
        msg.net_sent = net_io_counters.bytes_sent / 1024 / 1024
        msg.net_recv = net_io_counters.bytes_recv / 1024 / 1024

        # Read battery percentage
        try:
            with open(self.battery_path, "r") as f:
                bat_cap = int(f.read().strip())
            msg.battery_percent = float(bat_cap)
        except FileNotFoundError:
            # No battery hardware, set to -1 as indicator
            msg.battery_percent = -1.0
            self.get_logger().debug("No battery detected (BAT0 not found)")

        self.get_logger().info(f'发布:{str(msg)}')
        self.status_publisher_.publish(msg)

def main():
    rclpy.init()
    node = SysStatusPub('sys_status_pub')
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
