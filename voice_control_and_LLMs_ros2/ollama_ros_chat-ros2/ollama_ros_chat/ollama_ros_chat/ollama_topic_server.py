#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import json
import time
from typing import List, Dict, Optional

# 新增：使用官方 openai 包
import openai

class OllamaChatNode(Node):
    def __init__(self):
        super().__init__('ollama_topic_server')

        # 发布 / 订阅保持不变
        self.response_publisher = self.create_publisher(String, 'chat_response', 10)
        self.message_subscription = self.create_subscription(
            String, 'chat_message', self.message_callback, 10)

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

    # ------------ 改动 2：使用 client.models.list() ------------
    def initialize_models(self):
        try:
            models = self.client.models.list()
            self.available_models = [m.id for m in models.data]
            self.get_logger().info(f"Available models: {', '.join(self.available_models)}")
        except Exception as e:
            self.get_logger().error(f"Error getting models: {e}")
            self.available_models = []

    # -----------------------------------------------------------

    def select_model(self):
        if self.use_model:
            return  # 已经通过参数指定
        if not self.available_models:
            self.get_logger().error("No models available")
            return
        self.use_model = self.available_models[0]
        self.get_logger().info(f"Auto-selected model: {self.use_model}")

    def message_callback(self, msg):
        try:
            user_message = json.loads(msg.data).get('content', '')
            self.conversation_history.append({"role": "user", "content": user_message})
            self.get_logger().info(f"Received: {user_message}")

            ts = time.time()
            response_content = self.get_response(self.conversation_history)
            self.get_logger().info(f"Response ({time.time()-ts:.2f}s): {response_content}")

            if response_content:
                self.conversation_history.append(
                    {"role": "assistant", "content": response_content})
                self.conversation_history = self.process_data(self.conversation_history)
        except Exception as e:
            self.get_logger().error(f"Error processing message: {e}")

    # ------------ 改动 3：使用 openai.ChatCompletion ------------
    def get_response(self, messages: List[Dict[str, str]]) -> Optional[str]:
        try:
            resp = self.client.chat.completions.create(
                model=self.use_model,
                messages=messages,
                temperature=self.temperature,
                stream=self.stream
            )

            # 非流式
            if not self.stream:
                content = resp.choices[0].message.content
                self._publish_chunk(content, is_done=True)
                return content

            # 流式
            full = ""
            for chunk in resp:
                delta = chunk.choices[0].delta.content or ""
                full += delta
                self._publish_chunk(delta, is_done=chunk.choices[0].finish_reason is not None)
                if chunk.choices[0].finish_reason is not None:
                    return full
        except Exception as e:
            self.get_logger().error(f"OpenAI API error: {e}")
            return None
    # -----------------------------------------------------------

    def _publish_chunk(self, chunk: str, is_done: bool):
        out = String()
        out.data = json.dumps(
            {"content": chunk, "model": self.use_model, "is_done": is_done})
        self.response_publisher.publish(out)

    # 其余函数保持不变
    def process_data(self, data_list):
        if self.history_length <= 0:
            raise ValueError("History length must be positive")
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
