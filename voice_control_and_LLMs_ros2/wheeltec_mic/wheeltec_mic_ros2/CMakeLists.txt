cmake_minimum_required(VERSION 3.8)
project(wheeltec_mic_ros2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(
  include
)

link_directories(
  lib/arm64
)
#ament_index_register_resource("ros1_bridge_foreign_mapping")
install(DIRECTORY launch config feedback_voice
  DESTINATION share/${PROJECT_NAME})

# find dependencies
find_package(tf2_geometry_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(serial REQUIRED)
find_package(wheeltec_mic_msg REQUIRED)
find_package(turn_on_wheeltec_robot REQUIRED)
 
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

add_executable(call_recognition src/call_recognition.cpp)
ament_target_dependencies(call_recognition rclcpp std_msgs wheeltec_mic_msg)

add_executable(command_recognition src/command_recognition.cpp)
ament_target_dependencies(command_recognition rclcpp rclcpp_action std_msgs wheeltec_mic_msg geometry_msgs nav2_msgs)
target_link_libraries(command_recognition ${catkin_LIBRARIES} rt)

add_executable(node_feedback src/node_feedback.cpp)
ament_target_dependencies(node_feedback rclcpp std_msgs geometry_msgs)

add_executable(wheeltec_m2n src/wheeltec_m2n.cpp src/ros2_service_m2n.cpp)
ament_target_dependencies(wheeltec_m2n rclcpp std_msgs serial wheeltec_mic_msg)
target_link_libraries(wheeltec_m2n -ljsoncpp)

add_executable(wheeltec_mic src/wheeltec_mic.cpp src/ros2_service.cpp)
ament_target_dependencies(wheeltec_mic rclcpp std_msgs serial wheeltec_mic_msg)
target_link_libraries(wheeltec_mic -ljsoncpp)

add_executable(wheeltec_m07 src/wheeltec_m07.cpp)
ament_target_dependencies(wheeltec_m07 rclcpp std_msgs serial)

add_executable(voice_control src/voice_control.cpp)
ament_target_dependencies(voice_control rclcpp std_msgs wheeltec_mic_msg)
target_link_libraries(voice_control ${catkin_LIBRARIES} offline_record_lib rt dl pthread stdc++ asound samplerate)

add_executable(motion_control src/motion_control.cpp)
ament_target_dependencies(motion_control rclcpp rclcpp_action std_msgs wheeltec_mic_msg geometry_msgs turn_on_wheeltec_robot nav_msgs tf2_ros tf2_geometry_msgs nav2_msgs) 

install(TARGETS
  call_recognition
  command_recognition
  node_feedback
  wheeltec_mic
  wheeltec_m2n
  wheeltec_m07
  voice_control
  motion_control
  RUNTIME DESTINATION lib/${PROJECT_NAME}    
)

ament_package()
