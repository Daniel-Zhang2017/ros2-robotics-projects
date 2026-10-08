cmake_minimum_required(VERSION 3.5)
project(bodyreader)

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
#find_package(actionlib REQUIRED)
#find_package(actionlib_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(std_srvs REQUIRED)
find_package(bodyreader_msg REQUIRED)
find_package(image_transport REQUIRED)                         # CHANGE
find_package(cv_bridge REQUIRED)  
find_package(rosidl_default_generators REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
#rosidl_generate_interfaces(${PROJECT_NAME}
# )


link_directories(lib)


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

include_directories(
  include
)

link_directories(
  lib lib/Plugins lib/Plugins/openni2 lib/Plugins/obt/3rdparty/lib
)

#add_executable(wheeltec_robot_node src/wheeltec_robot.cpp src/Quaternion_Solution.cpp)
#ament_target_dependencies(wheeltec_robot_node tf2_ros tf2 tf2_geometry_msgs rclcpp std_msgs nav_msgs std_srvs sensor_msgs ackermann_msgs serial wheeltec_robot_msg)

add_executable(main src/main.cpp)
ament_target_dependencies(main rclcpp bodyreader_msg sensor_msgs)
target_link_libraries(main astra ${catkin_LIBRARIES} astra astra_core astra_core_api)

add_executable(bodydata_process src/bodydata_process.cpp)
ament_target_dependencies(bodydata_process rclcpp bodyreader_msg sensor_msgs std_msgs geometry_msgs)
target_link_libraries(bodydata_process astra astra_core astra_core_api)

#add_executable(color-test src/color-test.cpp)
#add_dependencies(color-test ${${PROJECT_NAME}_EXPORTED_TARGETS})
#target_link_libraries(color-test ${catkin_LIBRARIES} astra astra_core astra_core_api)

add_executable(follower src/follower.cpp)
ament_target_dependencies(follower rclcpp bodyreader_msg sensor_msgs std_msgs geometry_msgs)
target_link_libraries(follower astra astra_core astra_core_api)

add_executable(interaction src/interaction.cpp)
ament_target_dependencies(interaction rclcpp bodyreader_msg geometry_msgs)
target_link_libraries(interaction astra astra_core astra_core_api)


add_executable(feedback src/feedback.cpp)
ament_target_dependencies(feedback rclcpp bodyreader_msg geometry_msgs)
target_link_libraries(feedback astra astra_core astra_core_api)


add_executable(image_trans src/image_trans.cpp)
ament_target_dependencies(image_trans rclcpp bodyreader_msg sensor_msgs image_transport)
target_link_libraries(image_trans astra astra_core astra_core_api)


install(PROGRAMS scripts/display.py scripts/compressed.py DESTINATION lib/${PROJECT_NAME})

install(TARGETS
main bodydata_process follower interaction feedback image_trans

DESTINATION lib/${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
install(
  DIRECTORY launch audio
  DESTINATION share/${PROJECT_NAME}
)

ament_package()

