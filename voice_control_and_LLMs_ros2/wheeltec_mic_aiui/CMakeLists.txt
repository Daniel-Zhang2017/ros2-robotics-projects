cmake_minimum_required(VERSION 3.8)
project(wheeltec_mic_aiui)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories( /usr/include/alsa
                     ${CMAKE_CURRENT_LIST_DIR}/include
                     ${CMAKE_CURRENT_LIST_DIR}/src/utils
                     ${CMAKE_CURRENT_LIST_DIR}/src/jsoncpp)

install(DIRECTORY launch AIUI config
  DESTINATION share/${PROJECT_NAME})

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(serial REQUIRED)
find_package(ollama_ros_msgs REQUIRED)
find_package(wheeltec_mic_msg REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(turn_on_wheeltec_robot REQUIRED)

# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

file(GLOB_RECURSE SOURCE_FILE
        "src/*.cpp")

link_directories(${CMAKE_CURRENT_LIST_DIR}/libs/arm64)

add_executable(${PROJECT_NAME}  ${SOURCE_FILE})
ament_target_dependencies(${PROJECT_NAME} rclcpp std_msgs)
target_link_libraries(${PROJECT_NAME} aiui asound samplerate pthread stdc++)

add_executable(node_feedback node/node_feedback.cpp)
ament_target_dependencies(node_feedback rclcpp std_msgs geometry_msgs)

add_executable(command_recognition node/command_recognition.cpp)
ament_target_dependencies(command_recognition rclcpp rclcpp_action std_msgs wheeltec_mic_msg geometry_msgs nav2_msgs)

add_executable(wheeltec_mic node/wheeltec_mic.cpp node/ros2_service.cpp)
ament_target_dependencies(wheeltec_mic rclcpp std_msgs serial wheeltec_mic_msg)
target_link_libraries(wheeltec_mic -ljsoncpp)

add_executable(wheeltecM07 node/wheeltec_m07.cpp)
ament_target_dependencies(wheeltecM07 rclcpp std_msgs)

add_executable(wheeltecM2N node/wheeltec_m2n.cpp node/ros2_service_m2n.cpp)
ament_target_dependencies(wheeltecM2N rclcpp std_msgs serial wheeltec_mic_msg)
target_link_libraries(wheeltecM2N -ljsoncpp)

add_executable(motion_control node/motion_control.cpp)
ament_target_dependencies(motion_control rclcpp rclcpp_action std_msgs wheeltec_mic_msg geometry_msgs turn_on_wheeltec_robot nav_msgs tf2_ros tf2_geometry_msgs nav2_msgs) 

add_executable(chat_service  node/chat_service.cpp)
ament_target_dependencies(chat_service rclcpp std_msgs ollama_ros_msgs)

install(TARGETS
  ${PROJECT_NAME}
  node_feedback
  command_recognition
  wheeltec_mic
  motion_control
  chat_service
  wheeltecM07
  wheeltecM2N
  RUNTIME DESTINATION lib/${PROJECT_NAME}    
)

ament_package()
