/**
 * @file   calibration_parser.cpp
 * @author Ugo Cupcic <ugo@shadowrobot.com>, Contact <contact@shadowrobot.com>
 * @date   Thu May 13 11:41:56 2010
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
 * @brief This is where the calibration matrix is read from a file, stored and where the actual mapping take place.
 *
 *
 */

#include <ros/ros.h>

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include "sr_remappers/calibration_parser.h"
#include <sstream>

using namespace std;

const std::string CalibrationParser::default_path = "/etc/robot/mappings/default_mapping";

CalibrationParser::CalibrationParser(bool transpose) : transpose_(transpose)
{
    if (transpose_)
      ROS_INFO("Using transposed version of the mapping");
    ROS_WARN("No calibration path was specified, using default path");
    int ret = init(default_path);
    if (ret != 0)
      throw std::runtime_error("calibration parser initialization failed");
}

CalibrationParser::CalibrationParser(std::string path, bool transpose) : transpose_(transpose)
{
    if (transpose_)
      ROS_INFO("Using transposed version of the mapping");
    int ret = init(path);
    if (ret != 0)
      throw std::runtime_error("calibration parser initialization failed");
}

int CalibrationParser::init( std::string path )
{
    //reserve enough lines
    calibration_matrix.reserve(25);

    ifstream calibration_file;
    calibration_file.open(path.c_str());

    //can't find the file
    if( !calibration_file.is_open() )
    {
        ROS_ERROR("Couldn't open the file %s", path.c_str());
        return -1;
    }

    //we read the file and put all the data in this matrix
    std::vector<std::vector<double> > tmp_matrix;

    string line;
    int cur_line = 0;
    std::vector<double> double_line;//(splitted_string.size());
    int previous_line_size = -1;
    while( !calibration_file.eof() )
    {
        getline(calibration_file, line);
        ++cur_line;

        //remove leading and trailing whitespaces
        line = boost::algorithm::trim_copy(line);

        //ignore empty line
        if( line.size() == 0 )
            continue;

        //ignore comments
        if( line[0] == '#' )
            continue;

        std::vector<std::string> splitted_string;
        boost::split(splitted_string, line, boost::is_any_of("\t "));
        
        double_line.clear();
        for( unsigned int index_col = 0; index_col < splitted_string.size(); ++index_col)
        {
            try{
              double val = convertToDouble(splitted_string[index_col]);
              double_line.push_back(val);
            }
            catch (const std::runtime_error& e){
               ROS_DEBUG("skipping non-double value (%s) in line ", splitted_string[index_col].c_str());
            }
        }
        if (previous_line_size > -1)
        {
          if (previous_line_size != double_line.size())
          {
            ROS_ERROR("inconstent number of values between line %d (%d values) and %d (%lud values). Check the mapping",
                      cur_line-1, previous_line_size, cur_line, double_line.size());
            return -1;
          }
        }
        previous_line_size = double_line.size();
        calibration_matrix.push_back(double_line);
    }
    calibration_file.close();

    stringstream ss;
    ss << "mapping matrix, from glove to hand" << endl;
    for( unsigned int line = 0; line < calibration_matrix.size(); ++line )
    {
        for( unsigned int col = 0; col < calibration_matrix[0].size(); ++col )
        {
            ss << calibration_matrix[line][col] << " ";
        }
        ss << endl;
    }
    if (calibration_matrix.size()>0)
      ROS_INFO_STREAM("loaded a mapping matrix for " << calibration_matrix[0].size() << 
                      " input joints to " << calibration_matrix.size() <<  " output joints" );
    ROS_DEBUG("%s",ss.str().c_str());
    return 0;
}

std::vector<double> CalibrationParser::get_remapped_vector( std::vector<double> input_vector)
{
    std::vector<double> result;
    double tmp_value;
    if (transpose_) // columns-major multiplication
    {
      //check the size of the matrix
      if( input_vector.size() != calibration_matrix.size() )
      {
        ROS_ERROR_STREAM_THROTTLE(1.0, "The size of the given vector doesn't correspond to the mapping: received "
                         << input_vector.size()
                         << ", wanted "
                         << calibration_matrix.size());
          return std::vector<double>(calibration_matrix[0].size());
      }
      result.resize(calibration_matrix[0].size());

      for( unsigned int col = 0; col < calibration_matrix[0].size(); ++col )
      {
          tmp_value = 0.0;
          for( unsigned int index_vec = 0; index_vec < calibration_matrix.size(); ++index_vec )
          {
              tmp_value += (input_vector[index_vec] * calibration_matrix[index_vec][col]);
          }
          result[col] = tmp_value;
      }
    }
    else // row-major multiplication (standard matrix multiplication)
    {
      if( calibration_matrix.size() > 0)
      {
        //check the size of the matrix
        if( input_vector.size() != calibration_matrix[0].size() )
        {
          ROS_ERROR_STREAM_THROTTLE(1.0, "The size of the given vector doesn't correspond to the mapping: received "
                           << input_vector.size()
                           << ", wanted "
                           << calibration_matrix[0].size());
            return std::vector<double>(calibration_matrix.size());
        }
        result.resize(calibration_matrix.size());

        for( unsigned int line = 0; line < calibration_matrix.size(); ++line )
        {
            tmp_value = 0.0;
            for( unsigned int index_vec = 0; index_vec < calibration_matrix[0].size(); ++index_vec )
            {
                tmp_value += (input_vector[index_vec] * calibration_matrix[line][index_vec]);
            }
            result[line] = tmp_value;
        }
      }
    }

    return result;
}
