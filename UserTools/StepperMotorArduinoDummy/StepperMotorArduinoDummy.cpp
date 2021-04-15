#include "StepperMotorArduinoDummy.h"

StepperMotorArduinoDummy::StepperMotorArduinoDummy():Tool()
{

}

bool StepperMotorArduinoDummy::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="") m_variables.Initialise(configfile);
  
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  if(!m_variables.Get("verbose", m_verbose)) m_verbose = 1;
  if(!m_variables.Get("n_motor", n_motor)) n_motor = 0;
  if(!m_variables.Get("steps_per_unit", steps_per_unit)) steps_per_unit = 1;
  m_variables.Get("USB_port", USB_port);

  if (EstablishUSB()) {
    SetStepsPerUnit(steps_per_unit);
    GetCurrentPosition(curr_pos);
    return true;
  }
  else
    return false;
}

bool StepperMotorArduinoDummy::Execute()
{

  switch (m_data->mode)
  {
    case state::move:
      Move(m_data->coord_angle);
      break;
  }

  return true;
}

bool StepperMotorArduinoDummy::Finalise()
{
  Log("StepperMotorArduinoDummy: Closing serial port", 1, m_verbose);

  return true;
}

bool StepperMotorArduinoDummy::Move(float pos)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(4) << fmod(pos, 360.);
  std::string pos_str = ss.str();

  Log("StepperMotorArduinoDummy: Moving to position "+pos_str, 1, m_verbose);

  curr_pos = pos;
  //GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduinoDummy::MoveRelative(float dist)
{
  std::string dist_str = std::to_string(dist);
  Log("StepperMotorArduinoDummy: Moving "+dist_str+" units relative to current position", 1, m_verbose);
  float new_pos = curr_pos + dist;
  Move(new_pos);

  return true;
}

bool StepperMotorArduinoDummy::Home()
{
  Log("StepperMotorArduinoDummy: Homing", 1, m_verbose);
  curr_pos = 0;

  //GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduinoDummy::SetZero()
{
  Log("StepperMotorArduinoDummy: Setting current position to zero", 1, m_verbose);

  curr_pos = 0;
  //GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduinoDummy::SetStepsPerUnit(int steps)
{
  std::string steps_str = std::to_string(steps);
  Log("StepperMotorArduinoDummy: Setting steps per unit to "+steps_str, 1, m_verbose);

  return true;
}

bool StepperMotorArduinoDummy::GetCurrentPosition(float &position)
{
  position = curr_pos;

  return true;
}

bool StepperMotorArduinoDummy::EstablishUSB()
{

  Log("StepperMotorArduinoDummy: Connecting to USB serial port " + USB_port, 1, m_verbose);

  return true;
}