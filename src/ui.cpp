/**
 * @file ui.cpp
 * @brief Interface between user and robot controller
 * @author Alex Thanaphon Leonardi
 * @version 1.0.0
 * @date 19/04/2022
 *
 * @details \n
 * 
 *
 * Subscribes to: <BR>
 * - /controller_stateinfo
 * - /ui_inputasync
 *
 * Publishes to: <BR>
 * - /user_goalpoints
 * - /move_base/cancel
 * - /controller_manual
 * - /controller_assisted
 * - /ui_inputrequest
 *
 * Description :
 *
 * This node acts as the interface between the user and the robot. It communicates
 * with the controller node to control the robot in 3 ways: autonomous driving,
 * manual driving or assisted driving. To ensure the program does not block while
 * waiting on certain types of input, this node works with the rt1a3_inputasync
 * node.
 */

#include <unistd.h>
#include <termios.h>
#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <move_base_msgs/MoveBaseGoal.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Bool.h>
#include <std_msgs/String.h>
#include <geometry_msgs/Point.h>
#include <actionlib_msgs/GoalID.h>
#include "rt1-assignment3/utils.h"

/**
 * @return 1 (autonomous), 2 (manual) or 3 (assisted)
 *
 * Queries user for their choice between autonomous, manual or assisted driving
 */
int getUserChoice ();

/**
 * @param msg Message to be printed
 *
 * Prints information about the controller state
 */
void printControllerInfo (const std_msgs::String::ConstPtr& msg);

/**
 * @param event Timeout event
 *
 * Globally flag that the robot has timed out
 */
void timeoutTimerCallback (const ros::TimerEvent& event);

/**
 * @param msg Input received
 *
 * Gets input asynchronously through node "rt1a3_inputasync"
 */
void getInputAsync (const std_msgs::String::ConstPtr& msg);

/// Used for utils.h/displayText(), delay between each printed character (in microseconds)
const int TEXT_DELAY = 25000;
bool isComplete;
bool isAutonomous;

/// Publisher to allow cancelling of goal
static ros::Publisher pubGoalPointCancel;

int main (int argc, char **argv) {
  double inputX;
  double inputY;
  int inputChoice;
  int userInput;
  bool isUserDeciding = true;
  geometry_msgs::Point msgGoalPoint; // Goal point for autonomous driving
  std_msgs::Bool msgBool; // Manual driving on/off

  ros::init(argc, argv, "rt1a3_ui");
  ros::NodeHandle nh;
  ros::Publisher pubGoalPoint; // Publishes goal points, will be read by controller node
  ros::Publisher pubManualDrive;
  ros::Publisher pubAssistedDrive;
  ros::Publisher pubInputRequest;
  ros::Subscriber subControllerInfo;
  ros::Subscriber subInputAsync;

  pubGoalPoint = nh.advertise<geometry_msgs::Point>("user_goalpoints", 1);
  pubGoalPointCancel = nh.advertise<actionlib_msgs::GoalID>("move_base/cancel", 1);
  pubManualDrive = nh.advertise<std_msgs::Bool>("controller_manual", 1);
  pubAssistedDrive = nh.advertise<std_msgs::Bool>("controller_assisted", 1);
  pubInputRequest = nh.advertise<std_msgs::Empty>("ui_inputrequest", 1);
  subControllerInfo = nh.subscribe("controller_stateinfo", 10, printControllerInfo);
  subInputAsync = nh.subscribe("ui_inputasync", 1, getInputAsync);
  isAutonomous = false;

  ros::Duration(1).sleep(); // in case there are error messages
  clearTerminal();

  // Get user input on what to do. Then, publish the appropriate information to
  // the controller node.
  ROS_INFO("Waiting for user input");
  while (isUserDeciding) {
    inputChoice = getUserChoice();

    switch(inputChoice) {
      case 49: // Choice 1: Autonomous Goal Point
      {
        double actionTimeout;

        isUserDeciding = false;
        isComplete = false;
        isAutonomous = true;
        if (nh.getParam("/rt1a3_action_timeout", actionTimeout)) {
          // ROS_INFO("Action timeout successfully retrieved from parameter server");
        } else {
          ROS_ERROR("Failed to retrieve action timeout from parameter server");
        }

        clearTerminal();

        displayText("Mode: ", TEXT_DELAY);
        terminalColor(36, true);
        displayText("Autonomous Goal Point\n", TEXT_DELAY);
        terminalColor(37, true);
        isUserDeciding = true;
        // Get user input
        while (isUserDeciding) {
          std::string inputX_str;
          std::string inputY_str;
          displayText("Input goal point coordinates\n", TEXT_DELAY);
          displayText("X: ", TEXT_DELAY);
          std::cin >> inputX_str;
          displayText("Y: ", TEXT_DELAY);
          std::cin >> inputY_str;

          // Check if input is numeric or not
          if (!isStringNumeric(inputX_str) || !isStringNumeric(inputY_str)) {
            displayText("Invalid input. Please insert numeric coordinates!\n", TEXT_DELAY);
          } else {
            // Good input!
            isUserDeciding = false;
            inputX = std::stoi(inputX_str);
            inputY = std::stoi(inputY_str);
          }
        }

        // Publish new goal point for controller node
        msgGoalPoint.x = inputX;
        msgGoalPoint.y = inputY;
        pubGoalPoint.publish(msgGoalPoint);

        clearInputBuffer();
        clearTerminal();
        terminalColor(37, false);
        displayText("Driving...\n", TEXT_DELAY);
        terminalColor(36, false);
        displayText("\nPress q to cancel the goal, or any other key to continue.\n", TEXT_DELAY);

        // Start counter to timeout
        ros::Timer timeoutTimer = nh.createTimer(ros::Duration(actionTimeout), timeoutTimerCallback);
        // User is allowed to cancel the robot's goal point: listen for input!
        // The input must be asynchronous!

        std_msgs::Empty msgEmpty;
        pubInputRequest.publish(msgEmpty);

        while (true) {
          if (isComplete) {
            displayText("\nTerminating program...\n", TEXT_DELAY);
            ros::Duration(2).sleep();
            return 0; // Exit because we are done!
          }

          ros::spinOnce();
          ros::Duration(1).sleep();
        }

        break;
      }

      case 50: // Choice 2: Manual Driving
        isUserDeciding = false;
        msgBool.data = true;

        clearTerminal();

        displayText("Mode: ", TEXT_DELAY);
        terminalColor(36, true);
        displayText("Manual Drive\n", TEXT_DELAY);

        pubManualDrive.publish(msgBool);

        break;

      case 51: // Choice 3: Assisted Driving
        isUserDeciding = false;
        msgBool.data = true;

        clearTerminal();

        displayText("Mode: ", TEXT_DELAY);
        terminalColor(36, true);
        displayText("Assisted Drive\n", TEXT_DELAY);

        pubAssistedDrive.publish(msgBool);

        break;

      default:
        terminalColor(41, true);
        displayText("\nInvalid. Please select a valid option.\n", TEXT_DELAY);
        terminalColor(37, false);
        ros::Duration(1).sleep();
        inputChoice = getUserChoice();
        break;
    }
  }

  ros::spin();

  return 0;
}

int getUserChoice () {
  int inputChoice;

  clearTerminal();
  terminalColor(37, false);
  displayText("Choose one of the following options: \n", TEXT_DELAY);
  terminalColor(32, false);
  displayText("1. Autonomously reach coordinates\n", TEXT_DELAY);
  displayText("2. Manual driving\n", TEXT_DELAY);
  displayText("3. Assisted driving\n", TEXT_DELAY);
  terminalColor(37, false);
  inputChoice = detectKeyPress();

  return inputChoice;
}

void printControllerInfo (const std_msgs::String::ConstPtr& msg) {
  if (!isComplete) {
    clearTerminal();

    if (isAutonomous) {
      terminalColor(37, false);
      std::cout << "Driving...\n";
      terminalColor(36, false);
      std::cout << "\nPress q to cancel the goal, or any other key to continue.\n\n";
    }

    terminalColor(32, true);
    std::cout << "[Controller]: ";
    terminalColor(37, false);
    std::cout << msg->data;
    // terminalColor(36, false);
    // std::cout << "\n\nPress ESC to select a new driving mode";

    fflush(stdout);
  }
}

void timeoutTimerCallback(const ros::TimerEvent& event) {
  isComplete = true;

  clearTerminal();
  terminalColor(31, true);
  std::cout << "Action timed out! (Are you sure the goal was reachable?)\n";
  fflush(stdout);
}

void getInputAsync (const std_msgs::String::ConstPtr& msg) {
  std::string inputStr;
  int inputASCII;
  actionlib_msgs::GoalID goalCancelID;

  inputStr = msg->data;
  inputASCII = (int) inputStr[0];

  if (!isComplete) {
    if (inputASCII == 113) {
      // "q" pressed - cancel goal!
      clearTerminal();
      terminalColor(37, false);
      ROS_INFO("Autonomous driving cancel request by user.");
      displayText("Sending goal cancel request...\n", TEXT_DELAY);

      // Setting fields for cancel request
      goalCancelID.stamp.sec = 0;
      goalCancelID.stamp.nsec = 0;
      goalCancelID.id = "";

      pubGoalPointCancel.publish(goalCancelID);
      terminalColor(32, false);
      displayText("\nGoal has been cancelled.\n", TEXT_DELAY);

      isComplete = true;
    } else {
      clearTerminal();
      terminalColor(37, false);
      displayText("Driving...\n", TEXT_DELAY);
      terminalColor(36, false);
      displayText("\nPress q to cancel the goal, or any other key to continue.\n", TEXT_DELAY);
    }
  }
}
