#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import json
import sys
import threading
import time
class ChatClientNode(Node):
    def __init__(self):
        super().__init__('ollama_topic_client')
        
        # 创建发布者和订阅者
        self.message_publisher = self.create_publisher(
            String, 
            'chat_message', 
            10
        )
        self.response_subscription = self.create_subscription(
            String,
            'chat_response',
            self.response_callback,
            10
        )
        self.get_logger().info('Chat Client Node initialized')
        self.current_response = ""
        self.is_done = True

    def response_callback(self, msg):
        """Handle incoming chat responses"""
        try:
            response_data = json.loads(msg.data)
            content = response_data.get('content', '')
            self.is_done = response_data.get('is_done', True)
            print(content, end='', flush=True)
            
        except Exception as e:
            self.get_logger().error(f"Error processing response: {e}")

    def send_message(self, message: str):
        """Send a chat message"""
        msg = String()
        msg.data = json.dumps({
            "content": message
        })
        self.message_publisher.publish(msg)
        self.is_done = False

def main(args=None):
    rclpy.init(args=args)
    client_node = ChatClientNode()
    
    # 创建一个线程来运行ROS2节点
    def spin_node():
        rclpy.spin(client_node)
    
    thread = threading.Thread(target=spin_node, daemon=True)
    thread.start()
    time.sleep(3)
    print("Chat Client Node is running")
    try:
        while True:
            if client_node.is_done:
                user_input = input("\nuser: ")
                if user_input.lower() == 'exit':
                    break
                client_node.send_message(user_input)
    except KeyboardInterrupt:
        print("\nProgram interrupted")
    finally:
        client_node.destroy_node()
        rclpy.shutdown()
        sys.exit(0)

if __name__ == '__main__':
    main()