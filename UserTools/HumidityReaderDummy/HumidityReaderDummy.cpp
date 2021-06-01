#include "HumidityReaderDummy.h"

HumidityReaderDummy::HumidityReaderDummy():Tool(), gaussian(20.,2.)
{

}

bool HumidityReaderDummy::Initialise(std::string configfile, DataModel &data)
{
  m_data = &data;
  m_log = m_data->Log;

  if(!m_data->vars.Get("verbose",m_verbose)) m_verbose = 1;
  m_data->vars.Get("humid_USBport", USB_port);

  if (EstablishUSB())
    return true;
  else
    return false;

}

bool HumidityReaderDummy::Execute()
{

  switch(m_data->mode)
  {
    case state::record:
      double humidity = GetHumidity();
      double temperature = GetTemperature();
      m_data->lab_humid = humidity;
      m_data->lab_temp = temperature;
      Log("HumidityReaderDummy: Humidity = "+std::to_string(humidity)+"%, Temperature = "+std::to_string(temperature)+"C", 1, m_verbose);
      break;
  }

  return true;
}

bool HumidityReaderDummy::Finalise()
{
  Log("HumidityReaderDummy: Closing serial port", 1, m_verbose);

  return true;
}

double HumidityReaderDummy::GetHumidity()
{

  double humidity = gaussian(generator);

  return humidity;
}

double HumidityReaderDummy::GetTemperature()
{

  double temperature = gaussian(generator);

  return temperature;
}

bool HumidityReaderDummy::EstablishUSB()
{

  Log("HumidityReaderDummy: Connecting to USB serial port " + USB_port, 1, m_verbose);

  return true;
}