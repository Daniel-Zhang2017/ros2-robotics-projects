#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from ollama_ros_msgs.srv import Chat   # 替换成你的 pkg/msg
import json
import time
from typing import List, Dict, Optional

import openai   # <-- 新增

class OllamaChatNode(Node):
    def __init__(self):
        super().__init__('ollama_server')

        # 创建服务
        self.chat_service = self.create_service(
            Chat, 'chat_service', self.handle_chat_request)

        # ------------------------------------------
        # ---- 参数声明（带默认值） ----
        self.declare_parameter('base_url',   'http://localhost:11434/v1')
        self.declare_parameter('api_key',    'ollama')
        self.declare_parameter('use_model',  'deepseek-r1')          # 空字符串表示自动选择
        self.declare_parameter('stream',     False)
        self.declare_parameter('temperature',0.5)
        self.declare_parameter('history_length', 10)

        # ---- 读取参数 ----
        self.base_url   = self.get_parameter('base_url').value
        self.api_key    = self.get_parameter('api_key').value
        self.stream     = self.get_parameter('stream').value
        self.temperature= self.get_parameter('temperature').value
        self.history_length = self.get_parameter('history_length').value
        self.use_model  = self.get_parameter('use_model').value
        
        # ---- 用参数创建 openai 客户端 ----
        self.client = openai.OpenAI(base_url=self.base_url, api_key=self.api_key)

        # ---- 模型选择：优先用参数，否则自动 ----
        self.initialize_models()   # 拉取可用列表
        self.use_model = self.get_parameter('use_model').value
        if not self.use_model:
            self.select_model()        # 自动选第一个
        else:
            self.get_logger().info(f'Using forced model: {self.use_model}')
        # ---- 初始化对话历史 ----
        self.conversation_history = [
            {"role": "system", "content": "You are a helpful assistant"}]
        self.get_logger().info('Ollama(OpenAI-compat) Chat Server initialized')

    # ------------ 拉取模型列表 ------------
    def initialize_models(self):
        try:
            models = self.client.models.list()
            self.available_models = [m.id for m in models.data]
            self.get_logger().info(f"Available models: {', '.join(self.available_models)}")
        except Exception as e:
            self.get_logger().error(f"Error getting models: {e}")
            self.available_models = []

    def select_model(self):
        if self.use_model:
            return  # 已经通过参数指定
        if not self.available_models:
            self.get_logger().error("No models available")
            return
        self.use_model = self.available_models[0]
        self.get_logger().info(f"Auto-selected model: {self.use_model}")

    # ------------ 服务回调 ------------
    def handle_chat_request(self, request, response):
        try:
            user_message = request.content
            self.conversation_history.append({"role": "user", "content": user_message})
            self.get_logger().info(f"Received: {user_message}")

            ts = time.time()
            reply = self.get_response(self.conversation_history)
            self.get_logger().info(f"Response ({time.time()-ts:.2f}s): {reply}")

            if reply:
                self.conversation_history.append({"role": "assistant", "content": reply})
                self.conversation_history = self.process_data(self.conversation_history)

                response.content = reply
                response.model   = self.use_model
                response.is_done = True
            else:
                response.content = "Error processing request"
                response.model   = self.use_model
                response.is_done = False
        except Exception as e:
            self.get_logger().error(f"Error processing request: {e}")
            response.content = "Error processing request"
            response.model   = self.use_model or ""
            response.is_done = False
        return response

    # ------------ 真正调用大模型 ------------
    def get_response(self, messages: List[Dict[str, str]]) -> Optional[str]:
        try:
            if not self.stream:
                # 非流式
                resp = self.client.chat.completions.create(
                    model=self.use_model,
                    messages=messages,
                    temperature=self.temperature,
                    stream=False
                )
                return resp.choices[0].message.content

            # 流式（如需）
            full = ""
            for chunk in self.client.chat.completions.create(
                    model=self.use_model,
                    messages=messages,
                    temperature=self.temperature,
                    stream=True
                    ):
                delta = chunk.choices[0].delta.content or ""
                full += delta
            return full

        except Exception as e:
            self.get_logger().error(f"OpenAI API error: {e}")
            return None

    # ------------ 其他工具函数 ------------
    def process_data(self, data_list: List[Dict[str, str]]) -> List[Dict[str, str]]:
        return data_list[-self.history_length:]


def main(args=None):
    rclpy.init(args=args)
    node = OllamaChatNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
