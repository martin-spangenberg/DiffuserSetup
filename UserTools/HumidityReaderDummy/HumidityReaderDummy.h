#ifndef HumidityReaderDummy_H
#define HumidityReaderDummy_H

#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h> // Sleep
#include <cstdlib>
#include <cmath>
#include <random>

// Linux serial headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include "Tool.h"


/**
* \class HumidityReaderDummy
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/

class HumidityReaderDummy: public Tool {


 public:

  HumidityReaderDummy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  double GetHumidity();
  double GetTemperature();
  bool EstablishUSB();
  
 private:

  int m_verbose; // Verbosity
  std::string USB_port; // Name of USB port on system
  int serial_port;
  struct termios tty; // Termios struct, we call it 'tty' for convention

  bool ReadSerial(std::string &response);
  bool WriteSerial(std::string msg);

  std::default_random_engine generator;
  std::normal_distribution<double> gaussian;

};


#endif
