import rclpy
from rclpy.node import Node
from status_interfaces.msg import SystemStatus

class BatteryAlertNode(Node):
    def __init__(self):
        super().__init__('battery_alert_node')
        self.get_logger().info("✅ Battery Alert Node started, waiting for /sys_status messages")

        self.subscription = self.create_subscription(
            SystemStatus,
            'sys_status',
            self.alert_callback,
            10
        )
        self.subscription

        # Alert thresholds
        self.threshold_warn = 40.0
        self.threshold_critical = 20.0
        self.threshold_danger = 10.0

    def alert_callback(self, msg: SystemStatus):
        bat = msg.battery_percent
        self.get_logger().info(f"📥 Received msg, battery = {bat:.1f}%")

        if bat == -1.0:
            self.get_logger().info("ℹ️ No battery hardware detected, skip alert check")
            return

        if bat < self.threshold_danger:
            self.get_logger().fatal(f"🔴 DANGER! Battery very low: {bat:.1f}%")
        elif bat < self.threshold_critical:
            self.get_logger().error(f"🟠 CRITICAL! Battery low: {bat:.1f}%")
        elif bat < self.threshold_warn:
            self.get_logger().warn(f"🟡 WARNING! Battery getting low: {bat:.1f}%")
        else:
            self.get_logger().info(f"✅ Battery OK: {bat:.1f}%")

def main():
    rclpy.init()
    node = BatteryAlertNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Node interrupted")
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
