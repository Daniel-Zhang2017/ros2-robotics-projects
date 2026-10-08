cmake_minimum_required(VERSION 3.5)
project(bodyreader_msg)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Body.msg"
  "msg/Bodylist.msg"
  "msg/Bodyposture.msg"
  "msg/Joint.msg"
  "msg/Lockedcharrgb.msg"
  "msg/Lockedmaskwh.msg"
  "msg/Maskdata.msg"
  "msg/Vector2f.msg"
  "msg/Vector3f.msg"
  DEPENDENCIES std_msgs
  ADD_LINTER_TESTS
 )

ament_export_dependencies(rosidl_default_runtime)

ament_package()

