#ifndef Scheduler_H
#define Scheduler_H

#include <string>
#include <iostream>
#include <msgpack.hpp>
#include <fstream>
#include <streambuf>

#include "Tool.h"


/**
 * \class Scheduler
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class Scheduler: public Tool {

 public:

  Scheduler(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  std::vector<std::tuple<double,double>> ParseRanges(std::string rangesString);
  bool UpdateMotorCoords();
  zmq::message_t ZMQCreateWaveformMessage(double angle, double ypos, std::vector<double> waveform_PMT, std::vector<double> waveform_PD);

 private:

  int m_verbose;
  zmq::socket_t *zmqsocket_send;
  zmq::socket_t *zmqsocket_recv;
  zmq::context_t *context;

  state nextState, lastState;
  std::map<state, std::string> stateName;

  std::ifstream *file;

  // Struct to iterate through defined coordinate ranges
  struct RangeIterator {
    double stepSize;
    std::vector<std::tuple<double,double>> ranges;
    int currRangeNum;
    double currPos;
    int direction;

    bool Initialise(double ss, std::vector<std::tuple<double,double>> ra)
    {
      stepSize = ss;
      ranges = ra;
      currRangeNum = 0;
      currPos = std::get<0>(ranges.at(0));
      direction = (std::get<0>(ranges.at(0)) < std::get<1>(ranges.at(0)))? 1 : -1;
      return true;
    }

    bool NextPos()
    {
      currPos = currPos + stepSize*direction;

      if((currPos - std::get<1>(ranges.at(currRangeNum)))*direction > 0)
      {
        currRangeNum += 1;
        if(currRangeNum > ranges.size()-1)
        {
          Initialise(stepSize, ranges);
          return false;
        }
        currPos = std::get<0>(ranges.at(currRangeNum));
        direction = (std::get<0>(ranges.at(currRangeNum)) < std::get<1>(ranges.at(currRangeNum)))? 1 : -1;
      }

      return true;
    }

    double GetPos()
    {
      return currPos;
    }

  } m_iterAngle, m_iterY;



};


#endif
