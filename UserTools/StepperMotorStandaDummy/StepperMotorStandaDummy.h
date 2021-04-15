#ifndef StepperMotorStandaDummy_H
#define StepperMotorStandaDummy_H

#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ximc.h>

#include "Tool.h"


/**
* \class StepperMotorStandaDummy
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/
class StepperMotorStandaDummy: public Tool {


 public:

  StepperMotorStandaDummy(); ///< Simple constructor
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
  int steps_per_unit; // Number of motor steps per chosen distance unit
  //float curr_pos; // Current position of the motor
  device_t device;
  std::string m_device_name;
  engine_settings_t engine_settings;
  status_t status;
  status_calb_t status_calb;
  calibration_t calibration;
  const char* units = "mm";
  double curr_pos;

  bool GetCurrentPosition(float &position);

};


#endif
