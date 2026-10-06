import rclpy
from rclpy.node import Node
from status_interfaces.msg import SystemStatus

class SysStatusSub(Node):
    def __init__(self):
        super().__init__('sys_status_sub')
        self.subscription = self.create_subscription(
            SystemStatus,
            'sys_status',
            self.listener_callback,
            10
        )
        self.subscription  # prevent unused variable warning

    def listener_callback(self, msg):
        self.get_logger().info(
            f"[Subscriber] Host:{msg.host_name} | CPU:{msg.cpu_percent:.1f}% | "
            f"Mem:{msg.memory_percent:.1f}% | Battery:{msg.battery_percent:.1f}%"
        )

def main():
    rclpy.init()
    node = SysStatusSub()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
