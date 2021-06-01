#include "StepperMotorStandaDummy.h"

StepperMotorStandaDummy::StepperMotorStandaDummy():Tool()
{

}

bool StepperMotorStandaDummy::Initialise(std::string configfile, DataModel &data)
{
  m_data = &data;
  m_log = m_data->Log;

  if(!m_data->vars.Get("verbose",m_verbose)) m_verbose = 1;
  if(!m_data->vars.Get("linmotor_steps_unit", steps_per_unit)) steps_per_unit = 1;
  m_data->vars.Get("linmotor_devicename", m_device_name);

  curr_pos = 0;

  if (EstablishUSB()) {
    SetStepsPerUnit(steps_per_unit);
    //GetCurrentPosition(curr_pos);
    return true;
  }
  else
    return false;

  return true;
}

bool StepperMotorStandaDummy::Execute()
{

  switch (m_data->mode)
  {
    case state::move:
      Move(m_data->coord_y);
      break;
  }

  return true;
}

bool StepperMotorStandaDummy::Finalise()
{
  Log("StepperMotorStandaDummy: Closing device", 1, m_verbose);

  return true;
}

bool StepperMotorStandaDummy::Move(float pos)
{
  Log("StepperMotorStandaDummy: Moving to position "+std::to_string(pos));
  curr_pos = pos;

  return true;
}

bool StepperMotorStandaDummy::MoveRelative(float dist)
{
  Log("StepperMotorStandaDummy: Moving "+std::to_string(dist)+" units relative to current position", 1, verbose);
  curr_pos = curr_pos + dist;

  return true;
}

bool StepperMotorStandaDummy::Home()
{
  Log("StepperMotorStandaDummy: Homing", 1, verbose);
  curr_pos = 0;

  return true;
}

bool StepperMotorStandaDummy::SetZero()
{
  Log("StepperMotorStandaDummy: Setting current position to zero", 1, verbose);
  curr_pos = 0;

  return true;
}

bool StepperMotorStandaDummy::SetStepsPerUnit(int steps)
{
  Log("StepperMotorStandaDummy: Setting steps per unit to "+std::to_string(steps), 1, verbose);
  calibration.A = 1./steps;

  return true;
}

bool StepperMotorStandaDummy::GetCurrentPosition(float &position)
{
  position = curr_pos;

  return true;
}

bool StepperMotorStandaDummy::EstablishUSB()
{

  Log("StepperMotorStandaDummy: Opening motor", 1, verbose);

  return true;
}