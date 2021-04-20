#include "FunctionGeneratorDummy.h"

FunctionGeneratorDummy::FunctionGeneratorDummy():Tool()
{

}

bool FunctionGeneratorDummy::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  if(!m_variables.Get("verbose", m_verbose)) m_verbose = 1;
  if(!m_variables.Get("channel", m_channel)) m_channel = 1;
  if(!m_variables.Get("shape", m_shape)) m_shape = "SQUARE";
  if(!m_variables.Get("cycles", m_cycles)) m_cycles = 1;
  if(!m_variables.Get("frequency", m_frequency)) m_frequency = 1.0;
  if(!m_variables.Get("Vmin", m_Vmin)) m_Vmin = 0.0;
  if(!m_variables.Get("Vmax", m_Vmax)) m_Vmax = 1.0;
  m_variables.Get("IPaddress", m_IPaddress);

  if (EstablishConnection())
  {
    InitSetup();
    SetShape(m_shape);
    SetNCycles(m_cycles);
    SetFrequencyHz(m_frequency);
    SetOutputVoltsMin(m_Vmin);
    SetOutputVoltsMax(m_Vmax);
    ActivateChannel(m_channel);

    m_data->pulse_rate = m_frequency;
    m_data->pulse_N = m_cycles;

    return true;
  }
  else
    return false;
}

bool FunctionGeneratorDummy::Execute()
{
  switch(m_data->mode)
  {
    case state::record:
      SendTrigger();
      sleep(5);
      break;
  }

  return true;
}

bool FunctionGeneratorDummy::Finalise()
{
  Log("FunctionGeneratorDummy: Finalising", 1, m_verbose);

  return true;
}

bool FunctionGeneratorDummy::InitSetup()
{
  Log("FunctionGeneratorDummy: Initial setup - output mode: BURST, trigger mode: EXTERNAL", 1, m_verbose);

  return true;
}

bool FunctionGeneratorDummy::ActivateChannel(int channel)
{
  Log("FunctionGeneratorDummy: Setting output channel to "+std::to_string(channel), 1, m_verbose);
  if (channel > 0 and channel < 3)
  {
    m_channel = channel;
    return true;
  }
  else
  {
    Log("FunctionGeneratorDummy: Invalid channel number selected", 1, m_verbose);
    return false;
  }
}

bool FunctionGeneratorDummy::SetShape(std::string shape)
{
  Log("FunctionGeneratorDummy: Setting function shape to "+shape, 1, m_verbose);
  m_shape = shape;
  return true;
}

bool FunctionGeneratorDummy::SetNCycles(int cycles)
{
  Log("FunctionGeneratorDummy: Setting cycles per burst to "+std::to_string(cycles), 1, m_verbose);
  m_cycles = cycles;
  return true;
}

bool FunctionGeneratorDummy::SetFrequencyHz(float frequency)
{
  Log("FunctionGeneratorDummy: Setting function frequency to "+std::to_string(frequency)+" Hz", 1, m_verbose);
  m_frequency = frequency;
  return true;
}

bool FunctionGeneratorDummy::SetOutputVoltsMin(float Vmin)
{
  Log("FunctionGeneratorDummy: Setting function minimum to "+std::to_string(Vmin)+" Volts", 1, m_verbose);
  m_Vmin = Vmin;
  return true;
}

bool FunctionGeneratorDummy::SetOutputVoltsMax(float Vmax)
{
  Log("FunctionGeneratorDummy: Setting function maximum to "+std::to_string(Vmax)+" Volts", 1, m_verbose);
  m_Vmax = Vmax;
  return true;
}

bool FunctionGeneratorDummy::SendTrigger()
{
  Log("FunctionGeneratorDummy: Sending trigger", 1, m_verbose);
  return true;
}

bool FunctionGeneratorDummy::EstablishConnection()
{
  Log("FunctionGeneratorDummy: Connecting to instrument with IP address "+m_IPaddress, 1, m_verbose);

  return true;
}