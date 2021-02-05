#include <fstream>
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "trajectory_loader/node.hpp"

TrajectoryLoaderNode::TrajectoryLoaderNode() : Node("trajectory_loader_node")
{
  rclcpp::Publisher<autoware_planning_msgs::msg::Trajectory>::SharedPtr pub_;
  //todo remove rcl
  // QoS setup
  static constexpr std::size_t queue_size = 1;
  rclcpp::QoS durable_qos(queue_size);
  durable_qos.transient_local();  // option for latching

  pub_ = this->create_publisher<autoware_planning_msgs::msg::Trajectory>("trajectory", 1);

  // param
  std::string file_name, frame_id;

  //pnh_.getParam("trajectory_file_name", file_name);
  //nh_.param("frame_id", frame_id, std::string("map"));
  file_name = this->declare_parameter("trajectory_file_name", std::string("trajectory_file_name"));
  RCLCPP_INFO(get_logger(), "file name is %s", file_name.c_str());
  frame_id = this->declare_parameter("frame_id", std::string("map"));

  csv file_data;
  association label_row_association_map;

  // load csv file
  rclcpp::Rate loop_rate(1);
  int c = 0;
  while (!loadData(file_name, label_row_association_map, file_data) && rclcpp::ok()) {
    loop_rate.sleep();
    c++;
    RCLCPP_INFO(get_logger(), "c = %i", c);
  }

  // publish
  std_msgs::msg::Header header;
  header.frame_id = frame_id;
  header.stamp = this->now();
  publish(header, label_row_association_map, file_data);
}

bool TrajectoryLoaderNode::publish(
  const std_msgs::msg::Header & header, const association & label_row_association_map,
  const csv & file_data)
{
  autoware_planning_msgs::msg::Trajectory msg;
  msg.header = header;
  for (size_t i = 0; i < file_data.size(); ++i) {
    autoware_planning_msgs::msg::TrajectoryPoint point;
    double roll, pitch, yaw;

    for (size_t j = 0; j < file_data.at(i).size(); ++j) {
      RCLCPP_INFO(get_logger(), "i = %zu, j = %zu", i, j);
      if (label_row_association_map.at("x") == (int)j) {
        point.pose.position.x = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("y") == (int)j) {
        point.pose.position.y = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("z") == (int)j) {
        point.pose.position.z = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("roll") == (int)j) {
        roll = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("pitch") == (int)j) {
        pitch = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("yaw") == (int)j) {
        yaw = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("linear_velocity") == (int)j) {
        point.twist.linear.x = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("angular_velocity") == (int)j) {
        point.twist.angular.z = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("linear_acceleration") == (int)j) {
        point.accel.linear.x = std::stof(file_data.at(i).at(j));
      } else if (label_row_association_map.at("angular_acceleration") == (int)j) {
        point.accel.angular.z = std::stof(file_data.at(i).at(j));
      }
    }
    tf2::Quaternion tf2_quaternion;
    tf2_quaternion.setRPY(roll, pitch, yaw);
    tf2::convert(point.pose.orientation, tf2_quaternion);
    msg.points.push_back(point);
  }

  pub_->publish(msg);
  return true;
}

bool TrajectoryLoaderNode::loadData(
  const std::string & file_name, association & label_row_association_map, csv & file_data)
{
  file_data.clear();
  label_row_association_map.clear();
  std::ifstream ifs(file_name);

  /*
   * open file
   */
  if (!ifs) {
    //todo name arg of get logger
    RCLCPP_ERROR(get_logger(), "%s cannot load", file_name.c_str());
    return false;
  }

  /*
   * create label-row association map
   */
  std::string line;
  if (std::getline(ifs, line)) {
    std::vector<std::string> str_vec = split(line, ',');
    for (size_t i = 0; i < str_vec.size(); ++i) {
      deleteUnit(str_vec.at(i));
      deleteHeadSpace(str_vec.at(i));
      label_row_association_map[str_vec.at(i)] = (int)i;
      RCLCPP_INFO(
        get_logger(), "label_row_association_map[str_vec.at(i)] = %i, str_vec.at(i) = %s",
        label_row_association_map[str_vec.at(i)], str_vec.at(i).c_str());
    }
  } else {
    RCLCPP_ERROR(get_logger(), "cannot create association map");
    return false;
  }

  /*
   * create file data
   */
  while (std::getline(ifs, line)) {
    std::vector<std::string> str_vec = split(line, ',');
    file_data.push_back(str_vec);
  }

  return true;
}

std::vector<std::string> TrajectoryLoaderNode::split(const std::string & input, char delimiter)
{
  std::istringstream stream(input);
  std::string field;
  std::vector<std::string> result;
  while (std::getline(stream, field, delimiter)) {
    result.push_back(field);
  }
  return result;
}

void TrajectoryLoaderNode::deleteHeadSpace(std::string & string)
{
  while (string.find_first_of(' ') == 0) {
    string.erase(string.begin());
    if (string.empty()) break;
  }
}

void TrajectoryLoaderNode::deleteUnit(std::string & string)
{
  size_t start_pos, end_pos;
  start_pos = string.find_first_of('[');
  end_pos = string.find_last_of(']');
  if (start_pos != std::string::npos && end_pos != std::string::npos && start_pos < end_pos) {
    string.erase(start_pos, (end_pos + 1) - start_pos);
  }
}
