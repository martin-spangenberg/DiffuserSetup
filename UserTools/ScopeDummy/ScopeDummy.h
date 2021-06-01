#ifndef ScopeDummy_H
#define ScopeDummy_H

#include "visa.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <cmath>
#include <random>
#define _USE_MATH_DEFINES

#include "Tool.h"


/**
* \class ScopeDummy
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/
class ScopeDummy: public Tool {


 public:

  ScopeDummy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool InitSetup(); // Initial setup parameters
  bool SendTrigger();
  bool EstablishConnection();
  bool GetWaveform(std::vector<double> &waveform);
  double GetDeltaT();
  
 private:

  int m_verbose; // Verbosity
  std::string m_IPaddress; // IP address of instrument
  ViSession m_resource_manager;
  ViSession m_instrument; // VISA instrument reference

  // Communication buffers
  const ViUInt32 buffer_size_B = 100000;
  ViChar buffer[100000];
  ViUInt32 io_bytes;

  std::default_random_engine generator;
  std::normal_distribution<double> gaussian;
  double scalingfactor;

};


#endif
