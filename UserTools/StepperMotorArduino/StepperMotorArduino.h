#ifndef _H
#define _H

#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h> // Sleep

// Linux serial headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include "Tool.h"


/**
* \class StepperMotorArduino
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/

class StepperMotorArduino: public Tool {

 public:

  StepperMotorArduino(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose. 
  bool Finalise(); ///< Finalise function used to clean up resorces.

  bool Move(float pos);
  bool MoveRelative(float dist);
  bool Home();
  bool SetZero();
  bool SetStepsPerUnit(int steps);
  bool EstablishUSB();

 private:

  int verbose; // Verbosity
  std::string USB_port; // Name of USB port on system
  int serial_port;
  int n_motor; // Motor number set on Arduino
  int steps_per_unit; // Number of motor steps per chosen distance unit
  struct termios tty; // Termios struct, we call it 'tty' for convention

  std::string ReadSerial(int serial_port);
  bool WriteSerial(int serial_port, std::string msg);

};


#endif
