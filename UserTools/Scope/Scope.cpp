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
  m_variables.Get("scope_IPaddress", m_IPaddress);

  if (EstablishConnection())
  {
    InitSetup();
    m_data->dt = GetDeltaT();
    return true;
  }
  else
    return false;
}

bool Scope::Execute()
{
  //for (int i=0; i<100; ++i)
  //{
  //  SendTrigger();
  //  sleep(1);
  //}

 switch(m_data->mode)
  {
    case state::record:
      Log("ScopeDummy: Getting waveform from scope", 1, m_verbose);

      std::vector<double> waveform;
      GetWaveform(waveform);

      Log("ScopeDummy: Waveform size: "+std::to_string(waveform.size()), 1, m_verbose);

      m_data->waveform_PMT = waveform;

      break;
  }


  //Log("Scope: Waiting for data averaging", 1, m_verbose);
  //sleep(30);
  //std::vector<float> waveform;
  //GetWaveform(waveform);

  //std::cout << "Waveform size: " << waveform.size() << std::endl;
  //for (int i=0; i<waveform.size(); ++i)
  //{
  //  std::cout << waveform.at(i) << std::endl;
  //}
  

  return true;
}

bool Scope::Finalise()
{
  Log("Scope: Finalising", 1, m_verbose);
  viClose(m_instrument);

  return true;
}

double Scope::GetDeltaT()
{
  std::string output;
  WriteVISA("WFMO:XIN?");
  ReadVISA(output);
  return std::stof(output);
}

bool Scope::GetWaveform(std::vector<double> &waveform)
{
  waveform.clear();
  std::string output;

  WriteVISA("WFMOutpre:YOFf?");
  ReadVISA(output);
  float yoff = std::stof(output); // Y offset - Must be subtracted as step 1

  WriteVISA("WFMO:YMU?");
  ReadVISA(output);
  float ymult = std::stof(output); // Y scale - Must be multiplied as step 2

  WriteVISA("WFMO:YZE?");
  ReadVISA(output);
  float yzero = std::stof(output); // Y zero position - Must be added as step 3

  WriteVISA("CURVE?");
  ViStatus status = viRead(m_instrument, (unsigned char*)buffer, buffer_size_B, &io_bytes);

  if(!CheckStatus(status)) return false;

  int size;
  if (io_bytes < buffer_size_B)
    size = io_bytes;
  else
    size = buffer_size_B;

  std::cout << "Size of buffer in bytes: " << io_bytes << std::endl;

  char numdigits[1];
  strncat(numdigits, &(buffer[1]), 1); // Second byte contains number of additional digits in header
  int startpos = atoi(numdigits) + 2;
  int entries = (size - startpos) / 2; // Each entry is 2 bytes long

  std::cout << "Number of entries: " << std::endl;

  int16_t point;
  //float point;
  for(int i=0; i<entries; ++i)
  {
    point = 0; // Reset point, otherwise strncat will not overwrite previous data
    //strncat((char*)&point, &(buffer[startpos+i*4]), 4); // Copy next four bytes to point
    strncat((char*)&point, &(buffer[startpos+i*2]), 2); // Copy next two bytes to point
    waveform.push_back((point - yoff) * ymult + yzero);
  }

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
  WriteVISA("CH1:TERMINATION 50");
  //WriteVISA("CH1:SCALE ") 1V/div 

  WriteVISA("CH2:BANDWIDTH FULL");
  WriteVISA("CH2:COUPLING DC");
  WriteVISA("CH2:LABEL:NAME \"PMT SIGNAL\"");
  WriteVISA("CH2:POS 0");
  WriteVISA("CH2:OFFSET 0");
  WriteVISA("CH2:TERMINATION 50");
  //WriteVISA("CH2:SCALE ") 1V/div

  WriteVISA("HORIZONTAL:FASTFRAME:STATE 0");
  //WriteVISA("HORIZONTAL:RECORDLENGTH 10000");
  WriteVISA("HORIZONTAL:MAIN:POSITION 20");
  WriteVISA("HORIZONTAL:RESOLUTION 10000");
  WriteVISA("HORIZONTAL:SCALE 10E-9");

  WriteVISA("ACQUIRE:MODE AVERAGE");
  WriteVISA("ACQUIRE:NUMAVG 100");

  WriteVISA("TRIGGER:A:EDGE:COUPLING DC");
  WriteVISA("TRIGGER:A:EDGE:SLOPE RISE");
  WriteVISA("TRIGGER:A:EDGE:SOURCE CH1");
  WriteVISA("TRIGGER:A:LEVEL 2.0");

  WriteVISA("DATA:ENCDG SRIBINARY"); //SFPBINARY / SRIBINARY / ASCII
  WriteVISA("DATA:SOURCE CH2");
  WriteVISA("DATA:START 1");
  WriteVISA("DATA:STOP 999999");

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
