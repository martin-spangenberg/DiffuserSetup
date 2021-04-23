#include "ScopeDummy.h"

ScopeDummy::ScopeDummy():Tool()
{

}

bool ScopeDummy::Initialise(std::string configfile, DataModel &data)
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

bool ScopeDummy::Execute()
{

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

  return true;
}

bool ScopeDummy::Finalise()
{
  Log("ScopeDummy: Finalising", 1, m_verbose);

  return true;
}

double ScopeDummy::GetDeltaT()
{
    return pow(10., -9);
}

bool ScopeDummy::GetWaveform(std::vector<double> &waveform)
{
  waveform.clear();
  std::string output;

  float yoff = 0; // Y offset - Must be subtracted as step 1

  float ymult = 1.; // Y scale - Must be multiplied as step 2

  float yzero = 0.; // Y zero position - Must be added as step 3

  int entries = 10000;
  double sigma = 100;
  double mean = 5000;
  double time;
  for(int i=0; i<entries; ++i)
  {
    time = i;
    waveform.push_back(1/(sigma*sqrt(2*M_PI))*exp(-(time-mean)*(time-mean)/(2*sigma*sigma)));
  }

  return true;
}

bool ScopeDummy::InitSetup()
{
  Log("ScopeDummy: Initial setup", 1, m_verbose);

  return true;
}

bool ScopeDummy::SendTrigger()
{
  Log("ScopeDummy: Sending trigger", 1, m_verbose);
}

bool ScopeDummy::EstablishConnection()
{
  Log("ScopeDummy: Connecting to instrument with IP address "+m_IPaddress, 1, m_verbose);

  return true;
}
