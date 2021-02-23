#include "FunctionGenerator.h"

FunctionGenerator::FunctionGenerator():Tool()
{

}

bool FunctionGenerator::Initialise(std::string configfile, DataModel &data)
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
    return true;
  }
  else
    return false;
}

bool FunctionGenerator::Execute()
{
  // Test of trigger: 10 bursts, one every 2 seconds
  // sleep(2);
  // for (int i=0; i<10; ++i)
  // {
  //   SendTrigger();
  //   sleep(2);
  // }

  std::string response;
  WriteVISA("*IDN?");
  ReadVISA(response);

  Log("Response = " + response, 1, m_verbose);

  return true;
}

bool FunctionGenerator::Finalise()
{
  Log("FunctionGenerator: Finalising", 1, m_verbose);
  viClose(m_instrument);

  return true;
}

bool FunctionGenerator::InitSetup()
{
  Log("FunctionGenerator: Initial setup - output mode: BURST, trigger mode: EXTERNAL", 1, m_verbose);
  bool result1 = WriteVISA("SOURCE"+std::to_string(m_channel)+":BURST:STATE ON"); //Burst
  bool result2 = WriteVISA("TRIGGER:SEQUENCE:SOURCE EXTERNAL");

  return result1 and result2;
}

bool FunctionGenerator::ActivateChannel(int channel)
{
  Log("FunctionGenerator: Setting output channel to "+std::to_string(channel), 1, m_verbose);
  if (channel > 0 and channel < 3)
  {
    m_channel = channel;
    return WriteVISA("OUTPUT"+std::to_string(m_channel)+":STATE ON");
  }
  else
  {
    Log("FunctionGenerator: Invalid channel number selected", 1, m_verbose);
    return false;
  }
}

bool FunctionGenerator::SetShape(std::string shape)
{
  Log("FunctionGenerator: Setting function shape to "+shape, 1, m_verbose);
  m_shape = shape;
  return WriteVISA("SOURCE"+std::to_string(m_channel)+":FUNCTION:SHAPE "+shape);
}

bool FunctionGenerator::SetNCycles(int cycles)
{
  Log("FunctionGenerator: Setting cycles per burst to "+std::to_string(cycles), 1, m_verbose);
  m_cycles = cycles;
  return WriteVISA("SOURCE"+std::to_string(m_channel)+":BURST:NCYCLES "+std::to_string(m_cycles));
}

bool FunctionGenerator::SetFrequencyHz(float frequency)
{
  Log("FunctionGenerator: Setting function frequency to "+std::to_string(frequency)+" Hz", 1, m_verbose);
  m_frequency = frequency;
  return WriteVISA("SOURCE"+std::to_string(m_channel)+":FREQUENCY "+std::to_string(m_frequency));
}

bool FunctionGenerator::SetOutputVoltsMin(float Vmin)
{
  Log("FunctionGenerator: Setting function minimum to "+std::to_string(Vmin)+" Volts", 1, m_verbose);
  m_Vmin = Vmin;
  return WriteVISA("SOURCE"+std::to_string(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:LOW "+std::to_string(Vmin)+"V");
}

bool FunctionGenerator::SetOutputVoltsMax(float Vmax)
{
  Log("FunctionGenerator: Setting function maximum to "+std::to_string(Vmax)+" Volts", 1, m_verbose);
  m_Vmax = Vmax;
  return WriteVISA("SOURCE"+std::to_string(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:HIGH "+std::to_string(Vmax)+"V");
}

bool FunctionGenerator::SendTrigger()
{
  Log("FunctionGenerator: Sending trigger", 1, m_verbose);
  return WriteVISA("*TRG");
}

bool FunctionGenerator::EstablishConnection()
{
  Log("FunctionGenerator: Connecting to instrument with IP address "+m_IPaddress, 1, m_verbose);
  std::string resource_str = "TCPIP::"+m_IPaddress+"::INSTR";
  int length = resource_str.length();
  char resource[length];
  strcpy(resource, resource_str.c_str());
  ViChar *resource_string = resource;
  ViAccessMode access_mode = VI_NULL;
  ViUInt32 timeout_ms = 5000;

  ViStatus status;
  status = viOpenDefaultRM(&m_resource_manager); // Get VISA resource manager
  if (status < VI_SUCCESS) {
    Log("FunctionGenerator: Could not open VISA resource manager", 1, m_verbose);
    return false;
  }

  status = viOpen(m_resource_manager, resource_string, access_mode, timeout_ms, &m_instrument); // Open instrument
  if (status < VI_SUCCESS) {
    Log("FunctionGenerator: Error connecting to instrument", 1, m_verbose);
    viStatusDesc(m_resource_manager, status, buffer);
    printf("%s\n", buffer);
    return false;
  }

  viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE, timeout_ms); // Set timeout on instrument io

  return true;
}

bool FunctionGenerator::CheckStatus(ViStatus status)
{
  if (status < VI_SUCCESS) {
    Log("FunctionGenerator: Error communicating with instrument", 1, m_verbose);
    viStatusDesc(m_resource_manager, status, buffer);
    printf("%s\n", buffer);
    return false;
  }
  else
    return true;
}

bool FunctionGenerator::WriteVISA(std::string command_str)
{
  int length = command_str.length();
  char command_cstr[length];
  strcpy(command_cstr, command_str.c_str());
  ViBuf command_vibuf = (unsigned char*)command_cstr;
  ViStatus status = viWrite(m_instrument, command_vibuf, (ViUInt32)strlen((const char*)command_vibuf), &io_bytes);
  
  return CheckStatus(status);
}

bool FunctionGenerator::ReadVISA(std::string &response)
{
  ViStatus status = viRead(m_instrument, (unsigned char*)buffer, buffer_size_B, &io_bytes);

  if(CheckStatus(status))
  {
    if (io_bytes < buffer_size_B) // Response is not null-terminated. Add '\0' at end.
      buffer[io_bytes] = '\0';
    else
      buffer[buffer_size_B] = '\0';

    response = std::string(buffer);
    return true;
  }
  else
    return false;
}