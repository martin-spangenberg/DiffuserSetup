#include "HumidityReader.h"

HumidityReader::HumidityReader():Tool()
{

}

bool HumidityReader::Initialise(std::string configfile, DataModel &data)
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

bool HumidityReader::Execute()
{

  float humidity = GetHumidity();
  float temperature = GetTemperature();

  std::cout << "Humidity = " << humidity << ", Temperature = " << temperature << std::endl;

  return true;
}

bool HumidityReader::Finalise()
{
  Log("HumidityReader: Closing serial port", 1, m_verbose);
  close(serial_port);

  return true;
}

float HumidityReader::GetHumidity()
{
  std::string output;
  WriteSerial("HUMID\n");
  ReadSerial(output);

  float humidity = std::stof(output);

  return humidity;
}

float HumidityReader::GetTemperature()
{
  std::string output;
  WriteSerial("TEMP\n");
  ReadSerial(output);

  float temperature = std::stof(output);

  return temperature;
}

bool HumidityReader::WriteSerial(std::string msg)
{
  usleep(100000);
  int len = msg.length();
  char msg_cstr[len];
  strcpy(msg_cstr, msg.c_str());
  write(serial_port, msg_cstr, sizeof(msg_cstr));

  return true;
}

bool HumidityReader::ReadSerial(std::string &response)
{
  char read_buf[512]; // Allocate memory for read buffer
  response = "";

  while(true)
  {
    usleep(1000);
    memset(&read_buf, '\0', sizeof(read_buf));
    int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
    
    if (num_bytes > 0)
    {
      response += std::string(read_buf);
    }
    else if (num_bytes < 0)
    {
      std::string error_text = strerror(errno);
      Log("HumidityReader: Error reading: "+error_text, 1, m_verbose);
      return false;
    }

    if (response.find("\n") != std::string::npos)
    {
      if (response[response.length()-1] == '\n') response.erase(response.length()-1);
      break;
    }
  }

  return true;
}

bool HumidityReader::EstablishUSB()
{
  int len = USB_port.length();
  char USB_port_cstr[len];
  strcpy(USB_port_cstr, USB_port.c_str());
  serial_port = open(USB_port_cstr, O_RDWR | O_NOCTTY | O_SYNC); // Open serial port.

  // Read in existing settings, and handle any error
  if(tcgetattr(serial_port, &tty) != 0)
  {
    std::string error_number = std::to_string(errno);
    std::string error_text = strerror(errno);
    Log("HumidityReader: Error "+error_number+" from tcgetattr: "+error_text);
    return false;
  }

  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
  tty.c_cflag |= CS8; // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO; // Disable echo
  tty.c_lflag &= ~ECHOE; // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo
  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
  // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
  // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be 9600
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  Log("HumidityReader: Connecting to USB serial port " + USB_port, 1, m_verbose);

  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
  {
    std::string error_number = std::to_string(errno);
    std::string error_text = strerror(errno);
    Log("HumidityReader: Error "+error_number+" from tcsetattr: "+error_text, 1, m_verbose);
    return false;
  }

  return true;
}