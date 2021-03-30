#ifndef Scope_H
#define Scope_H

#include "visa.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include "Tool.h"


/**
* \class Scope
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/
class Scope: public Tool {


 public:

  Scope(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool InitSetup(); // Initial setup parameters
  bool SendTrigger();
  bool EstablishConnection();
  bool GetDeltaT(float &deltaT);
  bool GetWaveform(std::vector<float> &waveform);

 private:

  bool CheckStatus(ViStatus status);
  bool WriteVISA(std::string command_str);
  bool ReadVISA(std::string &response);

  int m_verbose; // Verbosity
  std::string m_IPaddress; // IP address of instrument
  ViSession m_resource_manager;
  ViSession m_instrument; // VISA instrument reference

  // Communication buffers
  const ViUInt32 buffer_size_B = 100000;
  ViChar buffer[100000];
  ViUInt32 io_bytes;

};


#endif
