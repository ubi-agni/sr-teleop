/**
 * @file   humanhand_to_cyberglove_remapper.cpp
 * @author Ugo Cupcic <ugo@shadowrobot.com>, Contact <contact@shadowrobot.com>
 * @date   Thu May 13 09:44:52 2010
 *
 * @author Guillaume Walck <gwalck@techfak.uni-bielefeld.de>
 * @date   Fri Apr 28 10:20 2010
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
 * @brief This program remaps the cyberglove joint information to 
 * joint_states in of a human hand display
 *
 */

//ROS include
#include <ros/ros.h>

//generic include
#include <string>

//own .h
#include "sr_remappers/humanhand_to_cyberglove_remapper.h"
#include <sr_robot_msgs/joint.h>
#include <sensor_msgs/JointState.h>

using namespace ros;

namespace humanhand_to_cyberglove_remapper
{

const unsigned int HumanhandToCybergloveRemapper::number_hand_joints = 20;

HumanhandToCybergloveRemapper::HumanhandToCybergloveRemapper() :
    n_tilde("~")
{
    joints_names.resize(number_hand_joints);
    HumanhandToCybergloveRemapper::init_names();

    std::string param;
    std::string path;
    n_tilde.searchParam("cyberglove_mapping_path", param);
    n_tilde.param(param, path, std::string());
    bool transposed;
    n_tilde.param<bool>("transposed", transposed, true);
    try{
      calibration_parser = new CalibrationParser(path, transposed);
    }
    catch (const std::runtime_error& e){
      ROS_ERROR("Failed to instantiated calibration parser (%s) ", e.what());
      exit(-1);
    }
    ROS_INFO("Mapping file loaded for the Cyberglove: %s", path.c_str());

    std::string prefix;
    std::string searched_param;
    n_tilde.searchParam("cyberglove_prefix", searched_param);
    n_tilde.param(searched_param, prefix, std::string());

    std::string full_topic = prefix + "/calibrated/joint_states";
    std::string remapped_full_topic = prefix + "/remapped/joint_states";

    cyberglove_jointstates_sub = node.subscribe(full_topic, 10, &HumanhandToCybergloveRemapper::jointstatesCallback, this);
    js_pub = node.advertise<sensor_msgs::JointState> (remapped_full_topic, 5);
}

void HumanhandToCybergloveRemapper::init_names()
{
    // TODO(guihome): get the names from the mapping file (at the beginning of the line)
    joints_names[0] = "thumb_distal_joint";
    joints_names[1] = "thumb_middle_joint";
    joints_names[2] = "thumb_abduction_joint";
    joints_names[3] = "thumb_proximal_joint";
    joints_names[4] = "index_middle_joint";
    joints_names[5] = "index_distal_joint";
    joints_names[6] = "index_proximal_joint";
    joints_names[7] = "index_abduction_joint";
    joints_names[8] = "middle_middle_joint";
    joints_names[9] = "middle_distal_joint";
    joints_names[10] = "middle_proximal_joint";
    joints_names[11] = "middle_abduction_joint";
    joints_names[12] = "ring_middle_joint";
    joints_names[13] = "ring_distal_joint";
    joints_names[14] = "ring_proximal_joint";
    joints_names[15] = "ring_abduction_joint";
    joints_names[16] = "little_middle_joint";
    joints_names[17] = "little_distal_joint";
    joints_names[18] = "little_proximal_joint";
    joints_names[19] = "little_abduction_joint";

    js_msg.name = joints_names;
    js_msg.position.resize(number_hand_joints);
}

void HumanhandToCybergloveRemapper::jointstatesCallback( const sensor_msgs::JointStateConstPtr& msg )
{
    //Do conversion
    std::vector<double> vect = calibration_parser->get_remapped_vector(msg->position);

    //Process J4's
    getAbductionJoints(msg, vect);

    //Generate message
    for(unsigned int i = 0; i < number_hand_joints; ++i )
    {
        js_msg.position[i] = vect[i] / 180.0 * M_PI; //radians in joint_states
    }
    
    js_msg.header.stamp = ros::Time::now();
    js_pub.publish(js_msg);
}

void HumanhandToCybergloveRemapper::getAbductionJoints( const sensor_msgs::JointStateConstPtr& msg, std::vector<double>& vect)
{
  double middleIndexAb = msg->position[8];
  double ringMiddleAb = msg->position[11];
  double pinkieRingAb = msg->position[14];

  // if the abduction sensors are less than 0, it is an artifact of the calibration (we don't want to consider anything smaller than 0 for these sensors)
  if (middleIndexAb < 0.0)
    middleIndexAb = 0.0;
  if (ringMiddleAb < 0.0)
    ringMiddleAb = 0.0;
  if (pinkieRingAb < 0.0)
    pinkieRingAb = 0.0;

  //Add the 3 abduction angles to have an idea of where the centre lies
  double ab_total = middleIndexAb + ringMiddleAb +  pinkieRingAb;

  // code has flipped signed compared to shadow version because human hand has not different on J4's
  if (ab_total/2 < middleIndexAb) // If the centre lies between ff and mf
  {
    //index_abduction_joint
    vect[7] = -ab_total/2;
    //middle_abduction_joint
    vect[11] = middleIndexAb - ab_total/2;
    //ring_abduction_joint
    vect[15] = (ringMiddleAb + vect[11]);
    //little_abduction_joint
    vect[19] = pinkieRingAb - vect[15];
  }
  else if (ab_total/2 < middleIndexAb + ringMiddleAb) // If the centre lies between mf and rf
  {
    //middle_abduction_joint
    vect[11] = -(ab_total/2 - middleIndexAb);
    //index_abduction_joint
    vect[7] = -middleIndexAb + vect[11];
    //ring_abduction_joint
    vect[15] = (ringMiddleAb + vect[11]);
    //little_abduction_joint
    vect[19] = pinkieRingAb - vect[15];
  }
  else // If the centre lies between rf and lf
  {
    //little_abduction_joint
    vect[19] = ab_total/2;
    //ring_abduction_joint
    vect[15] = -pinkieRingAb - vect[19];
    //middle_abduction_joint
    vect[11] = -(ringMiddleAb + vect[15]);
    //index_abduction_joint
    vect[7] = -middleIndexAb + vect[11];
  }
}
}//end namespace
