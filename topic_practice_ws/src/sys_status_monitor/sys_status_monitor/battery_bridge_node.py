import rclpy
from rclpy.node import Node
from status_interfaces.msg import SystemStatus
from sensor_msgs.msg import BatteryState


class BatteryBridgeNode(Node):
    def __init__(self):
        super().__init__('battery_bridge_node')
        # Subscribe custom sys_status
        self.sub = self.create_subscription(
            SystemStatus,
            'sys_status',
            self.callback,
            10
        )
        # Publish standard BatteryState
        self.pub = self.create_publisher(BatteryState, '/battery_state', 10)
        self.get_logger().info("Battery Bridge Node started, waiting for /sys_status")

    def callback(self, msg: SystemStatus):
        # Add log to check if callback triggers
        self.get_logger().info(f"Received sys_status, battery: {msg.battery_percent}")

        bat_msg = BatteryState()
        bat_msg.header.stamp = self.get_clock().now().to_msg()
        bat_msg.header.frame_id = "battery_link"

        bat_percent = msg.battery_percent
        if bat_percent == -1.0:
            bat_msg.percentage = float('nan')
            bat_msg.power_supply_status = BatteryState.POWER_SUPPLY_STATUS_UNKNOWN
        else:
            bat_msg.percentage = bat_percent
            bat_msg.power_supply_status = BatteryState.POWER_SUPPLY_STATUS_DISCHARGING

        bat_msg.voltage = 12.0
        bat_msg.current = 0.0
        bat_msg.charge = 0.0
        bat_msg.capacity = 0.0
        bat_msg.design_capacity = 0.0
        bat_msg.power_supply_health = BatteryState.POWER_SUPPLY_HEALTH_GOOD

        self.pub.publish(bat_msg)
        self.get_logger().info(f"Published /battery_state: {bat_msg.percentage}")


def main():
    rclpy.init()
    node = BatteryBridgeNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
