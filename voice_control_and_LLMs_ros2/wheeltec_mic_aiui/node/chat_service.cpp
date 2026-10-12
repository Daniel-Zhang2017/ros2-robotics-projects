#include "chat_service.h"

/**************************************************************************
函数功能：识别结果sub回调函数
**************************************************************************/
void Chat_Node::voice_words_Callback(const std_msgs::msg::String::SharedPtr msg){
    std::string chat_text = msg->data;    //取传入数据
    sendMessage(chat_text);
}

/**************************************************************************
函数功能：对话服务请求发送
**************************************************************************/
void Chat_Node::sendMessage(const std::string& message) {
    auto request = std::make_shared<ollama_ros_msgs::srv::Chat::Request>();
    request->content = message;
    waiting_for_response_ = true;
    std::cout << "正在思索整理中..." << std::endl;
    // 使用回调处理响应，不保存 future
    client_->async_send_request(
        request,
        [this](rclcpp::Client<ollama_ros_msgs::srv::Chat>::SharedFuture future) {
            this->response_callback(future);
        }
    );
}

/**************************************************************************
函数功能：对话服务response处理
**************************************************************************/
void Chat_Node::response_callback(rclcpp::Client<ollama_ros_msgs::srv::Chat>::SharedFuture future) {
    try {
        auto response = future.get();
        if (response) {
            std::string rmText = removeTags(response->content);
            std::cout << rmText << std::endl;
            std_msgs::msg::String result_text;
            result_text.data = rmText;
            chat_words_pub->publish(result_text);
            waiting_for_response_ = false;
        }
    } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Error processing response: %s", e.what());
    }
}

/**************************************************************************
函数功能：移除think标签
**************************************************************************/
std::string Chat_Node::removeTags(const std::string& input) {
    std::string result = input;
    size_t start_pos = 0;

    // 循环查找并移除 <think> 和 </think> 及其之间的内容
    while ((start_pos = result.find("<think>", start_pos)) != std::string::npos) {
        size_t end_pos = result.find("</think>", start_pos);
        if (end_pos == std::string::npos) {
            // 如果没有找到对应的结束标签，直接返回结果
            break;
        }
        // 计算需要移除的部分长度
        size_t length_to_remove = end_pos - start_pos + strlen("</think>");
        result.erase(start_pos, length_to_remove);
        // 更新搜索起点
        start_pos = start_pos; 
    }
    return result;
}

Chat_Node::Chat_Node(const std::string &node_name,
    const rclcpp::NodeOptions &options) : rclcpp::Node(node_name, options){
    RCLCPP_INFO(this->get_logger(),"%s node init!\n",node_name.c_str());

    /***服务客户端创建***/
    client_ = this->create_client<ollama_ros_msgs::srv::Chat>("chat_service");
    /***对话文本话题发布者创建***/
    chat_words_pub = this->create_publisher<std_msgs::msg::String>("feedback_words",10);
    /***识别结果话题订阅者创建***/
    voice_words_sub = this->create_subscription<std_msgs::msg::String>(
        "voice_words",10,std::bind(&Chat_Node::voice_words_Callback,this,std::placeholders::_1));

    // 等待服务可用
    while (!client_->wait_for_service(std::chrono::seconds(1))) {
        if (!rclcpp::ok()) {
            RCLCPP_ERROR(this->get_logger(), "Interrupted while waiting for service.");
            return;
        }
        RCLCPP_INFO(this->get_logger(), "Service not available, waiting again...");
    }
    
    RCLCPP_INFO(this->get_logger(), "Chat Client Node initialized");
}

Chat_Node::~Chat_Node(){
    RCLCPP_INFO(this->get_logger(),"Chat_Node over!\n");
}


int main(int argc, char** argv)
{
    rclcpp::init(argc,argv);
    auto node = std::make_shared<Chat_Node>("chat_node",rclcpp::NodeOptions());
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}