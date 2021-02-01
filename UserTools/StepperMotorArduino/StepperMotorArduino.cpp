#include "StepperMotorArduino.h"

StepperMotorArduino::StepperMotorArduino():Tool()
{

}

bool StepperMotorArduino::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="") m_variables.Initialise(configfile);
  
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  if(!m_variables.Get("verbose", m_verbose)) m_verbose = 1;
  if(!m_variables.Get("n_motor", n_motor)) n_motor = 0;
  if(!m_variables.Get("steps_per_unit", steps_per_unit)) steps_per_unit = 1;
  m_variables.Get("USB_port", USB_port);

  if (EstablishUSB()) {
    SetStepsPerUnit(steps_per_unit);
    GetCurrentPosition(curr_pos);
    return true;
  }
  else
    return false;
}

bool StepperMotorArduino::Execute()
{
  // Testing homing followed by move commands
  Log("Current position: "+std::to_string(curr_pos));
  Home();
  Log("Current position: "+std::to_string(curr_pos));
  SetZero();
  Log("Current position: "+std::to_string(curr_pos));
  Move(30.0);
  Log("Current position: "+std::to_string(curr_pos));
  MoveRelative(-15.0);
  Log("Current position: "+std::to_string(curr_pos));

  return true;
}

bool StepperMotorArduino::Finalise()
{
  Log("StepperMotorArduino: Closing serial port", 1, verbose);
  close(serial_port);

  return true;
}

bool StepperMotorArduino::Move(float pos)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(4) << fmod(pos, 360.);
  std::string pos_str = ss.str();

  Log("StepperMotorArduino: Moving to position "+pos_str, 1, verbose);
  std::string command = std::to_string(n_motor)+" MV "+pos_str+"\n";
  WriteSerial(serial_port, command);

  std::string output;
  while(true)
  {
    ReadSerial(serial_port, output);
    Log("Serial output: "+output, 5, verbose);
    if (output.find("MOVE DONE") != std::string::npos) break;
  }

  GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduino::MoveRelative(float dist)
{
  std::string dist_str = std::to_string(dist);
  Log("StepperMotorArduino: Moving "+dist_str+" units relative to current position", 1, verbose);
  float new_pos = curr_pos + dist;
  Move(new_pos);

  return true;
}

bool StepperMotorArduino::Home()
{
  Log("StepperMotorArduino: Homing", 1, verbose);
  std::string command = std::to_string(n_motor)+" HOME\n";
  WriteSerial(serial_port, command);

  std::string output;
  while(true)
  {
    ReadSerial(serial_port, output);
    if (output.find("HOME DONE") != std::string::npos) break;
  }

  GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduino::SetZero()
{
  Log("StepperMotorArduino: Setting current position to zero", 1, verbose);
  std::string command = std::to_string(n_motor)+" ZERO\n";
  WriteSerial(serial_port, command);
  std::string response;
  ReadSerial(serial_port, response);

  GetCurrentPosition(curr_pos);

  return true;
}

bool StepperMotorArduino::SetStepsPerUnit(int steps)
{
  std::string steps_str = std::to_string(steps);
  Log("StepperMotorArduino: Setting steps per unit to "+steps_str, 1, verbose);
  std::string command = std::to_string(n_motor)+" SPU "+steps_str+"\n";
  WriteSerial(serial_port, command);
  std::string response;
  ReadSerial(serial_port, response);

  return true;
}

bool StepperMotorArduino::GetCurrentPosition(float &position)
{
  std::string response;
  WriteSerial(serial_port, std::to_string(n_motor)+" POS?\n");
  ReadSerial(serial_port, response);
  position = std::stof(response);

  return true;
}

bool StepperMotorArduino::WriteSerial(int serial_port, std::string msg)
{
  usleep(100000);
  int len = msg.length();
  char msg_cstr[len];
  strcpy(msg_cstr, msg.c_str());
  write(serial_port, msg_cstr, sizeof(msg_cstr));

  return true;
}

bool StepperMotorArduino::ReadSerial(int serial_port, std::string &response)
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
      Log("StepperMotorArduino: Error reading: "+error_text, 1, verbose);
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

bool StepperMotorArduino::EstablishUSB()
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
    Log("StepperMotorArduino: Error "+error_number+" from tcgetattr: "+error_text);
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

  Log("StepperMotorArduino: Connecting to USB serial port " + USB_port, 1, verbose);

  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
  {
    std::string error_number = std::to_string(errno);
    std::string error_text = strerror(errno);
    Log("StepperMotorArduino: Error "+error_number+" from tcsetattr: "+error_text, 1, verbose);
    return false;
  }

  return true;
}