#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger  # 替换为你实际的服务消息类型
import json
import sys
import threading
import time
from ollama_ros_msgs.srv import Chat  # 替换为你实际的服务消息类型

class ChatClientNode(Node):
    def __init__(self):
        super().__init__('ollama_client')
        
        # 创建服务客户端
        self.client = self.create_client(Chat, 'chat_service')  # 替换为你实际的服务消息类型
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Service not available, waiting again...')
        
        self.get_logger().info('Chat Client Node initialized')
        self.current_response = ""

    def send_message(self, message: str):
        """Send a chat message"""
        request = Chat.Request()  # 替换为你实际的服务请求类型
        request.content = message
        # print(f"\nuser: {message}") 
        self.future = self.client.call_async(request)
    
    def response_callback(self):
        """Handle incoming chat responses"""
        try:
            response = self.future.result()
            content = response.content
            print(content, end='', flush=True)
            print("\nresponse done.")
        except Exception as e:
            self.get_logger().error(f"Error processing response: {e}")

def main(args=None):
    rclpy.init(args=args)
    ollama_client = ChatClientNode()
    time.sleep(3)
    print("Chat Client Node is running")
    try:
        while True:
            
            user_input = input("\nuser: ")
            if user_input.lower() == 'exit':
                break
            ollama_client.send_message(user_input)
            rclpy.spin_until_future_complete(ollama_client, ollama_client.future)
            if ollama_client.future.done():
                ollama_client.response_callback()
                    
    except KeyboardInterrupt:
        print("\nProgram interrupted")
    finally:
        ollama_client.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()