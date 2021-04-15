#ifndef FunctionGeneratorDummy_H
#define FunctionGeneratorDummy_H

#include "visa.h"
#include <string>
#include <iostream>
#include <unistd.h>

#include "Tool.h"


/**
* \class FunctionGeneratorDummy
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/
class FunctionGeneratorDummy: public Tool {


 public:

  FunctionGeneratorDummy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool InitSetup(); // Initial setup parameters
  bool ActivateChannel(int channel); // 1 or 2
  bool SetShape(std::string shape); // Options: SINUSOID, SQUARE, RAMP, PULSE
  bool SetNCycles(int n_cycles); // Number of cycles in a burst
  bool SetFrequencyHz(float frequency); // Set cycle frequency in Hz
  bool SetOutputVoltsMin(float Vmin);
  bool SetOutputVoltsMax(float Vmax);
  bool SendTrigger();
  bool EstablishConnection();

 private:

  int m_verbose; // Verbosity
  std::string m_IPaddress; // IP address of instrument
  ViSession m_resource_manager;
  ViSession m_instrument; // VISA instrument reference
  int m_channel; // Output channel to use
  std::string m_shape; // Shape of function. Valid options: SINUSOID, SQUARE, PULSE, RAMP 
  int m_cycles; // Number of function cycles in burst
  float m_frequency; // Frequency of function in Hz
  float m_Vmin;
  float m_Vmax;

  // Communication buffers
  const ViUInt32 buffer_size_B = 1000;
  ViChar buffer[1000];
  ViUInt32 io_bytes;



};


#endif