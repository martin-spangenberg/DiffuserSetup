#include "Scope.h"

Scope::Scope():Tool()
{

}

bool Scope::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose = 1;
  m_variables.Get("IPaddress", m_IPaddress);

  if (EstablishConnection())
  {
    return true;
  }
  else
    return false;

  return true;
}

bool Scope::Execute()
{

  

  return true;
}

bool Scope::Finalise()
{
  Log("FunctionGenerator: Finalising", 1, m_verbose);
  viClose(m_instrument);

  return true;
}

bool Scope::InitSetup()
{
  Log("Scope: Initial setup", 1, m_verbose);

  WriteVISA("CH1:BANDWIDTH FULL");
  WriteVISA("CH1:COUPLING DC");
  WriteVISA("CH1:LABEL:NAME \"LED TRIGGER\"");
  WriteVISA("CH1:POS -2.5");
  WriteVISA("CH1:OFFSET 0");
  WriteVISA("CH1:TERMINATION 50.0E+0");

  WriteVISA("CH1:BANDWIDTH FULL");
  WriteVISA("CH1:COUPLING DC");
  WriteVISA("CH1:LABEL:NAME \"PMT SIGNAL\"");
  WriteVISA("CH1:POS 0");
  WriteVISA("CH1:OFFSET 0");
  WriteVISA("CH1:TERMINATION 50.0E+0");

  WriteVISA("HORIZONTAL:FASTFRAME:STATE 0");
  WriteVISA("HORIZONTAL:RECORDLENGTH 10000");
  WriteVISA("HORIZONTAL:MAIN:POSITION 20");
  WriteVISA("HORIZONTAL:RESOLUTION 10000");
  WriteVISA("HORIZONTAL:SCALE 10"); // Time per division??

  WriteVISA("ACQUIRE:MODE AVERAGE");
  WriteVISA("ACQUIRE:NUMAVG 100");

  WriteVISA("TRIGGER:A:EDGE:COUPLING DC");
  WriteVISA("TRIGGER:A:EDGE:SLOPE RISE");
  WriteVISA("TRIGGER:A:EDGE:SOUCE CH1");
  WriteVISA("TRIGGER:A:LEVEL 2.0");

  WriteVISA("DATA:ENCDG FPBINARY");
  WriteVISA("DATA:SOURCE CH2");

  // Define measurements
  std::vector<std::string> meas
  {
    "AMPLITUDE",
    "RISE",
    "FALL",
    "AREA",
    "DELAY"
  };
  for (int i=0; i<5; ++i)
  {
    WriteVISA("MEASUREMENT:MEAS"+std::to_string(i+1)+":TYPE "+meas.at(i));
    WriteVISA("MEASUREMENT:MEAS"+std::to_string(i+1)+":STATE ON");
    WriteVISA("MEASUREMENT:MEAS"+std::to_string(i+1)+":SOURCE1 CH"+((i+1)<5 ? "2" : "1"));
    WriteVISA("MEASUREMENT:MEAS"+std::to_string(i+1)+":SOURCE2 CH"+((i+1)<5 ? "1" : "2"));
  }

  return true;
}

bool Scope::SendTrigger()
{
  Log("Scope: Sending trigger", 1, m_verbose);
  return WriteVISA("TRIGGER FORCE");
}

bool Scope::EstablishConnection()
{
  Log("Scope: Connecting to instrument with IP address "+m_IPaddress, 1, m_verbose);
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
    Log("Scope: Could not open VISA resource manager", 1, m_verbose);
    return false;
  }

  status = viOpen(m_resource_manager, resource_string, access_mode, timeout_ms, &m_instrument); // Open instrument
  if (status < VI_SUCCESS) {
    Log("Scope: Error connecting to instrument", 1, m_verbose);
    viStatusDesc(m_resource_manager, status, buffer);
    printf("%s\n", buffer);
    return false;
  }

  viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE, timeout_ms); // Set timeout on instrument io

  return true;
}

bool Scope::CheckStatus(ViStatus status)
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

bool Scope::WriteVISA(std::string command_str)
{
  int length = command_str.length();
  char command_cstr[length];
  strcpy(command_cstr, command_str.c_str());
  ViBuf command_vibuf = (unsigned char*)command_cstr;
  ViStatus status = viWrite(m_instrument, command_vibuf, (ViUInt32)strlen((const char*)command_vibuf), &io_bytes);

  return CheckStatus(status);
}

bool Scope::ReadVISA(std::string &response)
{
  ViStatus status = viRead(m_instrument, (unsigned char*)buffer, buffer_size_B, &io_bytes);

  // Response is not null-terminated. Add '\0' at end.
  if (io_bytes < buffer_size_B)
    buffer[io_bytes] = '\0';
  else
    buffer[buffer_size_B] = '\0';

  response = std::string(buffer);

  return CheckStatus(status);
}