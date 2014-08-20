/**
 * @file   shadowhand_to_cyberglove_remapper.cpp
 * @author Ugo Cupcic <ugo@shadowrobot.com>, Contact <contact@shadowrobot.com>
 * @date   Thu May 13 09:44:52 2010
 *
 *
 * Copyright 2011 Shadow Robot Company Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief This program remapps the force information contained in
 * /joint_states coming from the hand to the /cybergraspforces topic
 * used to control the cybergrasp.
 *
 *
 */

//ROS include
#include <ros/ros.h>

//generic include
#include <string>

//own .h
#include "sr_remappers/shadowhand_to_cyberglove_remapper.h"
#include <sr_robot_msgs/sendupdate.h>
#include <sr_robot_msgs/joint.h>

using namespace ros;
using namespace std;

namespace shadowhand_to_cyberglove_remapper
{

const unsigned int ShadowhandToCybergloveRemapper::number_hand_joints = 20;

ShadowhandToCybergloveRemapper::ShadowhandToCybergloveRemapper() :
  n_tilde("~"),
  joints_names(number_hand_joints)
{
  ShadowhandToCybergloveRemapper::init_names();

  string param;
  string path;
  n_tilde.searchParam("cyberglove_mapping_path", param);
  n_tilde.param(param, path, string());
  calibration_parser = new CalibrationParser(path);
  ROS_INFO("Mapping file loaded for the Cyberglove: %s", path.c_str());

  string prefix;
  string searched_param;
  n_tilde.searchParam("cyberglove_prefix", searched_param);
  n_tilde.param(searched_param, prefix, string());

  string full_topic = prefix + "/calibrated/joint_states";

  cyberglove_jointstates_sub = node.subscribe(full_topic, 10, &ShadowhandToCybergloveRemapper::jointstatesCallback, this);

  shadowhand_pub = n_tilde.advertise<sensor_msgs::JointState> ("joint_states", 1);
}

void ShadowhandToCybergloveRemapper::init_names()
{
  joints_names[0] = "THJ1";
  joints_names[1] = "THJ2";
  joints_names[2] = "THJ3";
  joints_names[3] = "THJ4";
  joints_names[4] = "THJ5";
  joints_names[5] = "FFJ0";
  joints_names[6] = "FFJ3";
  joints_names[7] = "FFJ4";
  joints_names[8] = "MFJ0";
  joints_names[9] = "MFJ3";
  joints_names[10] = "MFJ4";
  joints_names[11] = "RFJ0";
  joints_names[12] = "RFJ3";
  joints_names[13] = "RFJ4";
  joints_names[14] = "LFJ0";
  joints_names[15] = "LFJ3";
  joints_names[16] = "LFJ4";
  joints_names[17] = "LFJ5";
  joints_names[18] = "WRJ1";
  joints_names[19] = "WRJ2";

  for (size_t i = 0; i < joints_names.size(); ++i)
  {
    joint_state_msg.name.push_back(joints_names[i]);
    joint_state_msg.position.push_back(0.0);
  }
}

void ShadowhandToCybergloveRemapper::jointstatesCallback(const sensor_msgs::JointStateConstPtr& msg)
{
  //Do conversion
  vector<double> vect = calibration_parser->get_remapped_vector(msg->position);
  ros::Time now = ros::Time::now();
  joint_state_msg.header.stamp = now;

  for (unsigned int i = 0; i < number_hand_joints; ++i)
  {
    joint_state_msg.position[i] = vect[i];
  }

  shadowhand_pub.publish(joint_state_msg);
}
}//end namespace
