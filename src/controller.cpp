/**
  * @file controller.cpp
  * @brief Main robot controller
  * @author Alex Thanaphon Leonardi
  * @version 1.0.0
  * @date 19/04/2022
  *
  * @param [in] rt1a3_action_timeout Length of time in seconds before autonomous
  * driving action automatically times out
  *
  * @details \n
  * 
  *
  * Subscribes to: <BR>
  * - /user_goalpoints
  * - /controller_cmd_vel
  * - /controller_manual
  * - /controller_assisted
  * - /scan
  *
  * Publishes to: <BR>
  * - /controller_stateinfo
  * - /cmd_vel
  *
  * Description:
  *
  * This is the main robot controller node. It queries the rt1a3_inputasync node
  * for any user input and handles it appropriately.
  * If the user selected autonomous drive mode, then the controller will send the
  * goalpoint coordinates to the move_base node via an action.
  * If the user selected manual or assisted drive mode, then the controller will
  * remap all teleop_twist_keyboard messages so that they first pass through the
  * controller itself (for filtering). Then, the messages are forwarded to
  * move_base.
  * The controller is constantly sending information on the robot's current state
  * back to the rt1a3_ui (user interface) node via the /controller_stateinfo
  * topic.
  */

#include <unistd.h>
#include <sstream>
#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <actionlib_msgs/GoalID.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <move_base_msgs/MoveBaseGoal.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/String.h>
#include <std_msgs/Bool.h>
#include <std_srvs/Empty.h>
#include <sensor_msgs/LaserScan.h>

/**
  * @param msg Message to publish
  *
  * Publishes a msg to the "/controller_stateinfo" topic
  */
void sendInfo(std::string msg);

/**
 * @param msg Goalpoint coordinates to be reached
 *
 * Sends an action goal to the move_base node's action server which will
 * autonomously guide the robot to specified coordinates
 */
void autonomousDriveCallback(const geometry_msgs::Point::ConstPtr& msg);

/**
 * @param msg Incoming teleop_twist_keyboard command
 *
 * Listens to the "/cmd_vel" topic published by teleop_twist_keyboard
 * (remapped to "/controller_cmd_vel") and saves the commands in a global variable
 */
void userDriveCallback(const geometry_msgs::Twist::ConstPtr& msg);

/**
 * @param msg Boolean indicating manual drive (true) or not (false)
 *
 * Listens to the "/controller_manual" topic for a boolean, indicating whether
 * or not the drive mode should be manual
 */
void toggleManualDrive(const std_msgs::Bool::ConstPtr& msg);

/**
 * @param msg Boolean indicating assisted drive (true) or not (false)
 * Listens to the "/controller_assisted" topic for a boolean, indicating whether
 * or not the drive should be assisted
 */
void toggleAssistedDrive(const std_msgs::Bool::ConstPtr& msg);

/**
 * @param scaninfo Laser Scanner information
 *
 * Gets info from laser scanner on obstacles surrounding the robot and saves
 * it in struct minDistances
 */
void laserScanParser(const sensor_msgs::LaserScan::ConstPtr& scaninfo);

/**
 * Check laser scanner and modify current velocity to avoid any collisions
 */
void collisionAvoidance();

/// Distances of obstacles closest to the robot
struct minDistances {
  float left;
  float right;
};

static ros::Publisher pubStateInfo;
static ros::Publisher pubCmdVel;
static bool isManualDrive;
static bool isAssistedDrive;
static struct minDistances minDistances;
/// Velocity sent over from teleop_twist_keyboard
static geometry_msgs::Twist velFromTeleop;

/// Default timeout value (in seconds) - overwritten by ros param
const double ACTION_TIMEOUT_DEFAULT = 30.0;
/// Default brake threshold distance - overwritten by ros param
const double BRAKE_THRESHOLD_DEFAULT = 1.0;

int main (int argc, char **argv) {
  ros::init(argc, argv, "rt1a3_controller");

  ros::NodeHandle nh;
  ros::Subscriber subGoalPoint; // Receive new goal point coordinates from ui node
  ros::Subscriber subCmdVelRemapped; // teleop_twist_keyboard remapped to publish to new topic, listen to it!
  ros::Subscriber subManualDrive;
  ros::Subscriber subAssistedDrive;
  ros::Subscriber subScanner;

  // Set the timeout on the parameter server so UI can access it too
  if (!nh.hasParam("rt1a3_action_timeout")) {
    nh.setParam("/rt1a3_action_timeout", ACTION_TIMEOUT_DEFAULT);
  }

  // Set the brake threshold on the parameter server so it can be tweaked in runtime
  if (!nh.hasParam("rt1a3_brake_threshold")) {
    nh.setParam("/rt1a3_brake_threshold", BRAKE_THRESHOLD_DEFAULT);
  }

  isManualDrive = false;
  isAssistedDrive = false;
  pubStateInfo = nh.advertise<std_msgs::String>("controller_stateinfo", 10);
  pubCmdVel = nh.advertise<geometry_msgs::Twist>("cmd_vel", 10);
  subGoalPoint = nh.subscribe("user_goalpoints", 1, autonomousDriveCallback);
  subCmdVelRemapped = nh.subscribe("controller_cmd_vel", 10, userDriveCallback);
  subManualDrive = nh.subscribe("controller_manual", 1, toggleManualDrive);
  subAssistedDrive = nh.subscribe("controller_assisted", 1, toggleAssistedDrive);
  subScanner = nh.subscribe("scan", 1, laserScanParser);

  while (ros::ok()) {
    // If manual or assisted drive is chosen
    if (isManualDrive) {
      // MANUAL drive mode
      geometry_msgs::Twist newVel;
      newVel = velFromTeleop;
      pubCmdVel.publish(newVel);
    } else if (isAssistedDrive) {
      // Calculate collision avoidance!
      collisionAvoidance();
    }

    ros::Duration(0.1).sleep();
    ros::spinOnce();
  }

  return 0;
}

////////////////////////////////////////////////////////////

void sendInfo(std::string msg) {
  std_msgs::String stateInfoMsg;
  stateInfoMsg.data = msg;
  pubStateInfo.publish(stateInfoMsg);
}

void autonomousDriveCallback(const geometry_msgs::Point::ConstPtr& msg) {
  ros::NodeHandle nh;
  move_base_msgs::MoveBaseGoal moveBaseGoal;
  actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> moveBaseAC("move_base", true);
  double actionTimeout;

  // Get the timeout in seconds
  if (nh.getParam("/rt1a3_action_timeout", actionTimeout)) {
    // ROS_INFO("Action timeout successfully retrieved from parameter server");
  } else {
    ROS_ERROR("Failed to retrieve action timeout from parameter server");
  }

  ROS_INFO("New goal point received! Sending action to the move_base action server");

  // Setting fields for the goal action
  moveBaseGoal.target_pose.header.frame_id = "map";
  moveBaseGoal.target_pose.pose.orientation.w = 1;
  moveBaseGoal.target_pose.pose.position.x = msg->x;
  moveBaseGoal.target_pose.pose.position.y = msg->y;

  ROS_INFO("Waiting for action server to start");
  sendInfo("Waiting for robot to respond...");
  moveBaseAC.waitForServer();

  ROS_INFO("Action server started, sending goal");
  sendInfo("Driving towards requested coordinates...");
  moveBaseAC.sendGoal(moveBaseGoal);

  // If action does not finish before timeout, then cancel it and notify user
  if (moveBaseAC.waitForResult(ros::Duration(actionTimeout))) {
    actionlib::SimpleClientGoalState state = moveBaseAC.getState();
    ROS_INFO("Action finished: %s", state.toString().c_str());
    sendInfo("Goal has been successfully reached.");
  } else {
    ROS_INFO("Action timed out.");
    sendInfo("Timeout: goal has been automatically cancelled. (Are you sure the requested coordinates were reachable?)");
    moveBaseAC.cancelGoal();
  }
}

void userDriveCallback(const geometry_msgs::Twist::ConstPtr& msg) {
  velFromTeleop = *msg; // Save new velocity as global variable
}

void toggleManualDrive(const std_msgs::Bool::ConstPtr& msg) {
  if (msg->data == true) {
    // Toggle redirect
    isManualDrive = true;
    isAssistedDrive = false;
    sendInfo("Manual Drive mode enabled.");
  } else {
    // Stop redirecting
    isManualDrive = false;
    sendInfo("Manual Drive mode disabled.");
  }
}

void toggleAssistedDrive(const std_msgs::Bool::ConstPtr& msg) {
  if (msg->data == true) {
    // Toggle redirect
    isAssistedDrive = true;
    isManualDrive = false;
    sendInfo("Assisted Drive mode enabled.");
  } else {
    // Stop redirecting
    isAssistedDrive = false;
    sendInfo("Assisted Drive mode disabled.");
  }
}

void laserScanParser(const sensor_msgs::LaserScan::ConstPtr& scaninfo) {
  const int NUM_SECTORS = 2;
  int numElements;
  int numElementsSector;
  float leftDistMin;
  float rightDistMin;

  numElements = scaninfo->ranges.size();
  numElementsSector = numElements/NUM_SECTORS;
  // Temporarily take an element from each range
  leftDistMin = scaninfo->ranges[0];
  rightDistMin = scaninfo->ranges[numElements - 1];

  for (int i = 0; i < numElements; i++) {
    if (i < numElementsSector) {
      // FIRST sector
      if (scaninfo->ranges[i] < leftDistMin) {
        leftDistMin = scaninfo->ranges[i];
      }
    } else {
      // THIRD sector
      if (scaninfo->ranges[i] < rightDistMin) {
        rightDistMin = scaninfo->ranges[i];
      }
    }
  }

  minDistances.left = leftDistMin;
  minDistances.right = rightDistMin;
}

void collisionAvoidance() {
  // ASSISTED drive mode
  ros::NodeHandle nh;
  geometry_msgs::Twist newVel;
  double brakeThreshold;

  newVel = velFromTeleop;

  if (nh.getParam("/rt1a3_brake_threshold", brakeThreshold)) {
    // ROS_INFO("Brake threshold successfully retrieved from parameter server");
  } else {
    ROS_ERROR("Failed to retrieve brake threshold from parameter server");
  }

  // Correct user input
  if (minDistances.left <= brakeThreshold) {
    newVel.linear.x = velFromTeleop.linear.x/2;
    newVel.angular.z = 1; // Turn the other way
    sendInfo("Obstacle detected! Collision avoidance in progress.");
  } else if (minDistances.right <= brakeThreshold) {
    newVel.linear.x = velFromTeleop.linear.x/2;
    newVel.angular.z = -1; // Turn the other way
    sendInfo("Obstacle detected! Collision avoidance in progress.");
  } else {
    sendInfo("Listening to commands.");
  }

  pubCmdVel.publish(newVel);
}
