#ifndef __QUATERNION_SOLUTION_H_
#define __QUATERNION_SOLUTION_H_
#include "wheeltec_robot.h"

extern volatile uint8_t imu_freeze;
extern volatile float q0, q1, q2, q3;

float InvSqrt(float number);
void Quaternion_Solution(float gx, float gy, float gz, float ax, float ay, float az);
extern sensor_msgs::msg::Imu onboard_imu;  // External onboard IMU data // 外部板载IMU数据

#endif


