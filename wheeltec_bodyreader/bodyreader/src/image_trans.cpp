
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/image.hpp"
#include <sensor_msgs/msg/compressed_image.hpp>
#include "image_transport/image_transport.h"

rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub;

void ImageCallback(const sensor_msgs::msg::Image &msg)
{
	//printf("11111\n");
	image_pub->publish(msg);
}


int main(int argc, char **argv)
{
  std::string input_topic;
  std::string output_topic;

  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("CompressedImage");
  node->declare_parameter<std::string>("input_image_topic", "/body/image");
  node->declare_parameter<std::string>("output_image_topic", "/repub/body/image");
  node->get_parameter("input_image_topic", input_topic);
  node->get_parameter("output_image_topic", output_topic);

  image_transport::ImageTransport it(node);
  //image_transport::Publisher image_pub;
  auto image_sub = node->create_subscription<sensor_msgs::msg::Image>(input_topic, 1, ImageCallback);
  image_pub = node->create_publisher<sensor_msgs::msg::Image>(output_topic, 1);
  rclcpp::spin(node);
  return 0;
}

