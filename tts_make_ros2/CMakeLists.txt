cmake_minimum_required(VERSION 3.8)
project(tts)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(
  include
)

link_directories(
  libs/x64
)

install(DIRECTORY launch config 
  DESTINATION share/${PROJECT_NAME})

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

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

add_executable(${PROJECT_NAME}_node src/tts_make.cpp)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp std_msgs)
target_link_libraries(${PROJECT_NAME}_node ${catkin_LIBRARIES} msc rt dl pthread stdc++)

install(TARGETS
  ${PROJECT_NAME}_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}    
)

ament_package()
