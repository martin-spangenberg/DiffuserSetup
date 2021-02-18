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

