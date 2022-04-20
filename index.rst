.. rt1-assignment3 documentation master file, created by
   sphinx-quickstart on Tue Apr 19 11:46:40 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Research Track 1 Assignment 3 Documentation
===========================================
This is the third assignment of the "Research Track 1" course, for the Robotics Engineering degree, Università di Genova.

The assignment consists in writing two ROS nodes: a controller for a robot, and a UI. The user can choose one of three methods to command the robot (autonomously, manually, or in assisted driving mode).

A helper node was also implemented to get input asynchronously, i.e. without blocking the main nodes.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Index
......
* :ref:`genindex`

Quick Start
============

Installation
...........
The simulator requires ROS. Clone this repository into your ROS workspace's src/ folder.

.. code-block:: bash

   $ git clone https://www.github.com/ThanaphonLeonardi/rt1-assignment3

It also requires the final_assignment package, the slam_gmapping package, and the ROS navigation stack

.. code-block:: bash

   $ git clone https://www.github.com/CarmineD8/final_assignment

   $ git clone https://www.github.com/CarmineD8/slam_gmapping

   $ sudo apt-get install ros-noetic-navigation

Make sure to switch to the noetic branch on the git repositories!

Execution
.........
Launch the simulation.

.. code-block:: bash

   roslaunch final_assignment simulation_gmapping.launch

And the node that manages the movement

.. code-block:: bash

   roslaunch final_assignment move_base.launch

Finally, run the controller, ui, helper, and teleop_twist_keyboard node

.. code-block:: bash

   roslaunch rt1-assignment3 rt1a3.launch

   roslaunch rt1-assignment3 rt1a3_teleop.launch

Documentation
=============

Controller
...........
.. doxygenfile:: controller.cpp
   :project: rt1-assignment3

User Interface
..............
.. doxygenfile:: ui.cpp
   :project: rt1-assignment3

User Input Async
.................
.. doxygenfile:: inputasync.cpp
   :project: rt1-assignment3
