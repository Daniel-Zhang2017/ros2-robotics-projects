#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/string.h>

// 定义错误处理宏
#fn, check_for_errors(fn) \
  { \
    rcl_ret_t temp_rc = fn; \
    if ((temp_rc != RCL_RET_OK)) { \
      Serial.print("Failed status on line "); \
      Serial.print(__LINE__); \
      Serial.print(": "); \
      Serial.println(temp_rc); \
      for(;;); \
    } \
  }

rcl_publisher_t publisher;
std_msgs__msg__String msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rclc_executor_t executor;

unsigned long previous_millis = 0;
const long interval = 1000; // 发布间隔 1 秒
char buffer[50];

// 定时发布回调函数
void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  RCLC_UNUSED(timer);
  if (last_call_time != 0) {
    // 更新消息内容
    sprintf(buffer, "Hello from ESP32! Count: %ld", millis() / 1000);
    msg.data.data = buffer;
    msg.data.size = strlen(buffer);
    msg.data.capacity = sizeof(buffer);

    // 发布消息
    rcl_ret_t rc = rcl_publish(&publisher, &msg, NULL);
    if (rc == RCL_RET_OK) {
      Serial.println("Published message successfully");
    }
  }
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  
  // 设置 micro-ROS 串口传输（使用 Serial，默认连接电脑）
  set_micros_transports();
  
  // 等待连接建立
  delay(2000);

  allocator = rcl_get_default_allocator();

  // 初始化句柄
  check_for_errors(rclc_support_init(&support, 0, NULL, &allocator));

  // 创建节点，名称为 "esp32_node"
  check_for_errors(rclc_node_init_default(&node, "esp32_node", "", &support));

  // 创建发布者，话题名称为 "esp32_publisher"
  check_for_errors(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    "esp32_publisher"));

  // 创建定时器，周期为 1000ms (1秒)
  rcl_timer_t timer;
  const unsigned int timer_timeout = 1000;
  check_for_errors(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // 初始化执行器
  check_for_errors(rclc_executor_init(&executor, &support.context, 1, &allocator));
  check_for_errors(rclc_executor_add_timer(&executor, &timer));

  // 初始化消息字符串空间
  msg.data.data = (char *)::malloc(50);
  msg.data.size = 0;
  msg.data.capacity = 50;

  Serial.println("micro-ROS Node Initialized!");
}

void loop() {
  // 运行执行器处理回调
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
}