**Institution:** Università di Genova<br>
**Course:** MSc in Robotics Engineering<br>
**Subject:** Research Track 1<br>
**Author:** ***Alex Thanaphon Leonardi***<br>

# Assignment 3

## Introduction
This is the third assignment of the "Research Track 1" course, for the Robotics Engineering degree, Università di Genova.
The assignment consists in writing two ROS nodes: a controller for a robot, and a UI. The user can choose one of three methods to command the robot (autonomously, manually, or in assisted driving mode).
A helper node was also implemented to get input asynchronously, i.e. without blocking the main nodes.

## Running the program
### Installation
The simulator requires ROS. Clone this repository into your ROS workspace's src/ folder.
```
git clone https://www.github.com/ThanaphonLeonardi/rt1-assignment3
```
It also requires the final_assignment package, the slam_gmapping package, and the ROS navigation stack
```
git clone https://www.github.com/CarmineD8/final_assignment
git clone https://www.github.com/CarmineD8/slam_gmapping
sudo apt-get install ros-noetic-navigation
```
Make sure to switch to the noetic branch on the git repositories!
### Execution
Launch the simulation.
```
roslaunch final_assignment simulation_gmapping.launch
```
And the node that manages the movement
```
roslaunch final_assignment move_base.launch
```
Finally, run the controller, ui, helper, and teleop_twist_keyboard node
```
roslaunch rt1-assignment3 rt1a3.launch
roslaunch rt1-assignment3 rt1a3_teleop.launch
```

## Description
The robot can be controlled according to three modes:
1. Autonomous Drive: the robot can be given a pair of coordinates, towards which it will autonomously drive. The goal can be cancelled by pressing **q**.
2. Manual Drive: the robot can be manually controlled by an operator, using the **teleop_twist_keyboard** package. By choosing this option, the robot can be controlled via the terminal on which the rt1a3_teleop launch file was called, using the keys "uiojklm,.".
3. Assisted Drive: same as manual drive, except that the robot will now actively avoid all collisions, even superseding user input.

## Behind The Scenes
The **UI** node sends relevant information to the **controller** node, through the use of topics. It also communicates directly with the **move_base** node if the user requests goal cancellation, to ensure speedy response. The **controller** node not only receives input from the **UI** node, but also from the **teleop_twist_keyboard** node, which no longer publishes directly to the **cmd_vel** topic, but rather is remapped to publish to the **controller_cmd_vel** topic. This way, the **controller** node has the ability to block commands as necessary (in case of autonomous drive) or to filter the commands before they are published to the robot (in case of assisted drive).
The **UI** receives information from the **controller** node on the **controller_stateinfo** topic. This way, the user is always kept updated on the robot's state.
Both nodes set and get parameters from the parameter server, that can be modified in the launch file.

### Extras
The **roslaunch** file is set by default to print to the terminal. The program is capable of **logging** and can be set to do so, to the default ROS logs, by modifying the launch file accordingly.

## Node structure
Image

## Pseudocode
### controller node
```
  listen for UI node's commands

  if autonomous drive
    receive coordinates
    start driving action with coordinates

  elseif manual drive
    redirect teleop_twist_keyboard node to cmd_vel topic

  elseif assisted drive
    run collision avoidance on teleop_twist_keyboard's commands
    send filtered commands to cmd_vel

  send info to UI node
```

### user interface (ui) node
```
  listen for input

  while listening for input

    if key pressed == 1
      ask user for coordinates
      publish coordinates to controller
      listen for input
      while listening for input
        if key pressed == q
          cancel goal
          exit

    elseif key pressed == 2
      tell controller to go into manual drive mode
      print controller output

    elseif key pressed == 3
      tell controller to go into assisted drive mode
      print controller output

    else
      print "wrong input"

```
