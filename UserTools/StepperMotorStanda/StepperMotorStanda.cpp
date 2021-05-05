#include "StepperMotorStanda.h"

StepperMotorStanda::StepperMotorStanda():Tool()
{

}

bool StepperMotorStanda::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="") m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose = 1;
  if(!m_variables.Get("linmotor_steps_per_unit", steps_per_unit)) steps_per_unit = 1;
  m_variables.Get("linmotor_device_name", m_device_name);

  if (EstablishUSB()) {
    SetStepsPerUnit(steps_per_unit);
    //GetCurrentPosition(curr_pos);
    return true;
  }
  else
    return false;

  return true;
}

bool StepperMotorStanda::Execute()
{

  float position;

 switch (m_data->mode)
  {
    case state::move:

      //Move(m_data->coord_y);

      GetCurrentPosition(position);
      std::cout << "Position after move: " << position << std::endl;

      break;
  }

  

  //Home();
  //Move(20.);

  return true;
}

bool StepperMotorStanda::Finalise()
{
  Log("StepperMotorStanda: Closing device", 1, m_verbose);
  command_stop(device); // Soft stop motor
  close_device(&device); // Close device

  return true;
}

bool StepperMotorStanda::Move(float pos)
{
  Log("StepperMotorStanda: Moving to position "+std::to_string(pos));
  command_move_calb(device, pos, &calibration);
  command_wait_for_stop(device, 100);

  return true;
}

bool StepperMotorStanda::MoveRelative(float dist)
{
  Log("StepperMotorStanda: Moving "+std::to_string(dist)+" units relative to current position", 1, verbose);
  command_movr_calb(device, dist, &calibration);
  command_wait_for_stop(device, 100);

  return true;
}

bool StepperMotorStanda::Home()
{
  Log("StepperMotorStanda: Homing", 1, verbose);
  command_homezero(device);
  command_wait_for_stop(device, 100);


  return true;
}

bool StepperMotorStanda::SetZero()
{
  Log("StepperMotorStanda: Setting current position to zero", 1, verbose);
  command_zero(device);

  return true;
}

bool StepperMotorStanda::SetStepsPerUnit(int steps)
{
  Log("StepperMotorStanda: Setting steps per unit to "+std::to_string(steps), 1, verbose);
  calibration.A = 1./steps;

  return true;
}

bool StepperMotorStanda::GetCurrentPosition(float &position)
{
  get_status_calb( device, &status_calb, &calibration);
  position = status_calb.CurPosition;

  return true;
}

bool StepperMotorStanda::EstablishUSB()
{

  int names_count;
  char device_name[256];
  const int probe_flags = ENUMERATE_PROBE;
  //const int probe_flags = ENUMERATE_PROBE | ENUMERATE_NETWORK;
  const char* enumerate_hints = "addr=192.168.1.1,172.16.2.3";
  char ximc_version_str[32];
  device_enumeration_t devenum;

  //set_bindy_key("keyfile.sqlite"); // Only necessary if using network communication
  
  devenum = enumerate_devices(probe_flags, enumerate_hints); // Returns an opaque pointer to device enumeration data.
  names_count = get_device_count(devenum); // Get device count from device enumeration data

  //  Terminate if there are no connected devices
  if (names_count <= 0)
  {
    Log("StepperMotorStanda: No devices found", 1, verbose);
    free_enumerate_devices( devenum ); //  Free memory used by device enumeration data
    return false;
  }

  //for (int i=0; i<names_count; ++i)
  //{
  //  strcpy(device_name, get_device_name(devenum, i));
  //  printf("Device name: %s\n", device_name);
  //}

  //strcpy( device_name, get_device_name( devenum, 0 ) ); // Copy first found device name into a string
  free_enumerate_devices( devenum ); // Free memory used by device enumeration data

  strcpy(device_name, m_device_name.c_str());

  Log("StepperMotorStanda: Opening motor", 1, verbose);
  device = open_device(device_name); // Open device by device name

  Log("StepperMotorStanda: Getting status parameters", 1, verbose);
  get_status(device, &status);  // Read device status
  printf("position %d, encoder %lld, speed %d\n", status.CurPosition, status.EncPosition, status.CurSpeed);

  Log("StepperMotorStanda: Getting engine parameters", 1, verbose);
  get_engine_settings(device, &engine_settings); // Read engine settings
  printf( "voltage %d, current %d, speed %d\n", engine_settings.NomVoltage, engine_settings.NomCurrent, engine_settings.NomSpeed );

  // Read calibrated device status from a device
  get_status_calb( device, &status_calb, &calibration);
  printf( "calibrated position %.3f %s, calibrated speed %.3f %s/s\n", status_calb.CurPosition, units, status_calb.CurSpeed, units );

  calibration.MicrostepMode = engine_settings.MicrostepMode; // Set microstep mode to convert microsteps to calibrated units correctly

  return true;
}
