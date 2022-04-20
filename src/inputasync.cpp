/**
  * @file inputasync.cpp
  * @brief Capture user inputs and publish them.
  * @author Alex Thanaphon Leonardi
  * @version 1.0.0
  * @date 19/04/2022
  *
  * @details \n
  * 
  *
  * Subscribes to: <BR>
  * - /ui_inputrequest
  *
  * Publishes to: <BR>
  * - /ui_inputasync
  *
  * Description :
  *
  * This node asynchonrously captures user inputs and publishes them. Having an
  * extra node that can handle inputs is very important, because inputs are
  * generally blocking and it is not always feasible to interrupt everything until
  * the user inputs something.
  * To request user input, another node must simply publish any message to the
  * "/ui_inputrequest" topic, and the next user input will be published to the
  * "/ui_inputasync" topic.
  */

#include <ros/ros.h>
#include <iostream>
#include <string>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include "rt1-assignment3/utils.h"

/**
* @brief Detects next keypress.
* @param msg does nothing for this function
*
* Detects the next keypress (in whichever console this node is running in) and
* publishes it to "/ui_inputasync".
*/
void getInput (const std_msgs::Empty::ConstPtr& msg);

static ros::Publisher pubUI; ///< Publisher for "/ui_inputasync"

int main (int argc, char** argv) {
  ros::init(argc, argv, "rt1a3_inputasync");
  ros::NodeHandle nh;
  ros::Subscriber subUI;

  pubUI = nh.advertise<std_msgs::String>("ui_inputasync", 1);
  subUI = nh.subscribe("ui_inputrequest", 1, getInput);

  ros::spin();
}

void getInput (const std_msgs::Empty::ConstPtr& msg) {
  std_msgs::String input;
  std::string str;
  str = detectKeyPress();
  input.data = str;
  pubUI.publish(input);
}
