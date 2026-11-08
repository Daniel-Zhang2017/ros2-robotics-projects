#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/time_sync.h>
#include <sensor_msgs/msg/range.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ================= 配置 =================
#define TRIG_PIN        5
#define ECHO_PIN        18
#define PUBLISH_PERIOD_MS 50      // 20Hz 发布频率
#define QUEUE_SIZE      10

// ================= micro-ROS 句柄 =================
rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;
rcl_publisher_t publisher;
sensor_msgs__msg__Range range_msg;

// ================= FreeRTOS 队列 =================
QueueHandle_t distance_queue;

// ================= 超声波测距（高优先级任务） =================
static float measure_distance_cm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 等待 ECHO 变高，超时 5ms（对应约 85cm 以上无回波则放弃）
    unsigned long start_wait = micros();
    while (digitalRead(ECHO_PIN) == LOW) {
        if (micros() - start_wait > 5000) return -1.0f;
    }

    unsigned long pulse_start = micros();
    while (digitalRead(ECHO_PIN) == HIGH) {
        if (micros() - pulse_start > 25000) return -1.0f; // 超时
    }
    unsigned long pulse_end = micros();

    unsigned long duration = pulse_end - pulse_start;
    return (duration * 0.034f) / 2.0f; // cm
}

void sensor_task(void *pvParameters) {
    while (1) {
        float dist_cm = measure_distance_cm();
        if (dist_cm > 0) {
            // 非阻塞发送，队列满则丢弃最旧策略由 FreeRTOS 自动处理
            xQueueSend(distance_queue, &dist_cm, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));
    }
}

// ================= 发布任务（中优先级） =================
void publish_task(void *pvParameters) {
    float dist_cm;
    while (1) {
        if (xQueueReceive(distance_queue, &dist_cm, portMAX_DELAY) == pdTRUE) {
            // 填充消息
            range_msg.range = dist_cm / 100.0f; // 转换为米

            // 使用同步后的时间戳
            if (rmw_uros_epoch_synchronized()) {
                int64_t now_ns = rmw_uros_epoch_nanos();
                range_msg.header.stamp.sec = now_ns / 1000000000ULL;
                range_msg.header.stamp.nanosec = now_ns % 1000000000ULL;
            } else {
                // 降级：使用本地 millis（精度差，仅用于调试）
                uint32_t ms = millis();
                range_msg.header.stamp.sec = ms / 1000;
                range_msg.header.stamp.nanosec = (ms % 1000) * 1000000;
            }

            rcl_publish(&publisher, &range_msg, NULL);
        }
    }
}

// ================= micro-ROS 初始化 =================
bool init_micro_ros() {
    allocator = rcl_get_default_allocator();

    // 等待 Agent 连接
    if (rmw_uros_ping_agent(500, 3) != RMW_RET_OK) {
        return false;
    }

    // 初始化 support 与 node
    if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) return false;
    if (rclc_node_init_default(&node, "esp32_ultrasonic", "", &support) != RCL_RET_OK) return false;

    // 创建 publisher
    if (rclc_publisher_init_default(
        &publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/ultrasonic/range") != RCL_RET_OK) return false;

    // 填充静态消息字段
    range_msg.header.frame_id.data = "ultrasonic_link";
    range_msg.header.frame_id.size = strlen("ultrasonic_link");
    range_msg.header.frame_id.capacity = strlen("ultrasonic_link") + 1;
    range_msg.radiation_type = sensor_msgs__msg__Range__ULTRASOUND;
    range_msg.field_of_view = 0.785398f; // 45 度
    range_msg.min_range = 0.02f;
    range_msg.max_range = 4.0f;

    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // 配置 micro-ROS 串口传输（与 Agent 对应）
    set_microros_serial_transports(Serial);

    // 创建队列
    distance_queue = xQueueCreate(QUEUE_SIZE, sizeof(float));

    // 初始化 micro-ROS
    if (!init_micro_ros()) {
        Serial.println("micro-ROS init failed, restarting...");
        delay(2000);
        ESP.restart();
    }

    // 时钟同步（在 Agent 连接成功后执行一次）
    rmw_uros_sync_session(1000);
    if (rmw_uros_epoch_synchronized()) {
        Serial.println("Time sync OK");
    } else {
        Serial.println("Time sync FAILED, timestamps will be local");
    }

    // 创建 FreeRTOS 任务：测距优先级 3，发布优先级 2
    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(publish_task, "publish_task", 8192, NULL, 2, NULL, 0);
}

void loop() {
    // 空，所有工作由 FreeRTOS 任务完成
    vTaskDelay(pdMS_TO_TICKS(1000));
}