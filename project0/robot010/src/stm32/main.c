#include "main.h"
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/string.h>
#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

// Forward declarations for custom UART transport
bool cubemx_transport_open(struct uxrCustomTransport * transport);
bool cubemx_transport_close(struct uxrCustomTransport * transport);
size_t cubemx_transport_write(struct uxrCustomTransport * transport, const uint8_t * buf, size_t len, uint8_t * err);
size_t cubemx_transport_read(struct uxrCustomTransport * transport, uint8_t * buf, size_t len, int timeout, uint8_t * err);

rcl_publisher_t publisher;
std_msgs__msg__String msg;
char buffer[50];

void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
    RCLC_UNUSED(timer);
    if (last_call_time != 0) {
        sprintf(buffer, "Hello from STM32! Count: %lu", HAL_GetTick() / 1000);
        msg.data.data = buffer;
        msg.data.size = strlen(buffer);
        msg.data.capacity = sizeof(buffer);

        rcl_publish(&publisher, &msg, NULL);
    }
}

void micro_ros_task(void * argument) {
    // 1. Set custom UART transport
    rmw_uros_set_custom_transport(
        true,
        (void *) &huart2, // Assuming UART2 is connected to your PC/Agent
        cubemx_transport_open,
        cubemx_transport_close,
        cubemx_transport_write,
        cubemx_transport_read
    );

    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;
    rcl_ret_t rc;

    // 2. Initialize micro-ROS support
    rc = rclc_support_init(&support, 0, NULL, &allocator);

    // 3. Create ROS 2 Node
    rcl_node_t node;
    rc = rclc_node_init_default(&node, "stm32_node", "", &support);

    // 4. Create Publisher
    rc = rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        "stm32_publisher"
    );

    // 5. Create Timer (1000 ms period)
    rcl_timer_t timer;
    const unsigned int timer_timeout = 1000;
    rc = rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(timer_timeout),
        timer_callback
    );

    // 6. Create Executor
    rclc_executor_t executor;
    rc = rclc_executor_init(&executor, &support.context, 1, &allocator);
    rc = rclc_executor_add_timer(&executor, &timer);

    // Allocate buffer space for the message string
    msg.data.data = (chariconductor *) malloc(50);
    msg.data.size = 0;
    msg.data.capacity = 50;

    // 7. Main micro-ROS loop
    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        osDelay(10);
    }
}