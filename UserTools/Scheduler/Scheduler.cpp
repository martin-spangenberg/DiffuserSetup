#include "Scheduler.h"

Scheduler::Scheduler():Tool()
{

}

bool Scheduler::Initialise(std::string configfile, DataModel &data)
{
  if(configfile!="") m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data = &data;
  m_log = m_data->Log;

  std::string angleRangesString;
  std::string yRangesString;
  double stepSizeAngle;
  double stepSizeY;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose = 1;
  m_variables.Get("scheduler_stepSizeAngle",stepSizeAngle);
  m_variables.Get("scheduler_stepSizeY",stepSizeY);
  m_variables.Get("scheduler_angleRanges",angleRangesString);
  m_variables.Get("scheduler_yRanges",yRangesString);

  std::vector<std::tuple<double,double>> angleRanges = ParseRanges(angleRangesString);
  std::vector<std::tuple<double,double>> yRanges = ParseRanges(yRangesString);

  if(angleRanges.size() == 0 || yRanges.size() == 0)
  {
    Log("Scheduler: No scan ranges found. Exiting.", 1, m_verbose);
    return false;
  }

  iterAngle.Initialise(stepSizeAngle, angleRanges);
  iterY.Initialise(stepSizeY, yRanges);

  m_data->mode = state::idle;

  stateName[state::idle]     = "idle";
  stateName[state::init]     = "init";
  stateName[state::move]     = "move";
  stateName[state::record]   = "record";
  stateName[state::finalise] = "finalise";
  stateName[state::end]      = "end";

  context = new zmq::context_t(1);
  socket = new zmq::socket_t(*context, ZMQ_PUB);
  socket->bind("tcp://127.0.0.1:5555");

  return true;
}

bool Scheduler::Execute()
{

  //std::cout << "(y, angle) = (" << m_data->coord_y << ", " << m_data->coord_angle << ")" << std::endl;

  std::string mstring = "Draw";
  zmq::message_t msg(mstring.length()+1);

  switch (m_data->mode)
  {
    case state::idle:
      m_data->mode = state::init;
      break;

    case state::init:
      m_data->mode = state::move;
      m_data->coord_angle = iterAngle.GetPos();
      m_data->coord_y = iterY.GetPos();
      break;

    case state::move:
      m_data->mode = state::record;
      break;

    case state::record:
      m_data->mode = state::move;
      if(!UpdateMotorCoords())
        m_data->mode = state::finalise;
      snprintf ((char *) msg.data(), mstring.length()+1 , "%s" ,mstring.c_str());
      socket->send(msg);
      break;

    case state::finalise:
      m_data->vars.Set("StopLoop",1);
      m_data->mode = state::end;
      break;
  }

  Log("Scheduler: Starting mode "+stateName[m_data->mode], 1, m_verbose);

  return true;
}

bool Scheduler::Finalise()
{
  Log("Scheduler: Finalising", 1, m_verbose);
  return true;
}

std::vector<std::tuple<double,double>> Scheduler::ParseRanges(std::string rangesString)
{
  std::vector<std::tuple<double,double>> ranges;
  size_t start;
  size_t end = 0;
  std::string tempstring;
  while((start = rangesString.find_first_not_of("|", end)) != std::string::npos)
  {
    end = rangesString.find("|", start);
    tempstring = rangesString.substr(start, end - start);
    int delim_pos = tempstring.find(",");
    double first = std::stod(tempstring.substr(0,delim_pos));
    double last = std::stod(tempstring.substr(delim_pos+1));
    ranges.push_back(std::tuple<double,double>(first,last));
  }

  return ranges;
}

bool Scheduler::UpdateMotorCoords()
{
  bool validPosition = true;
  if(!iterAngle.NextPos())
  {
    if(!iterY.NextPos())
    {
      validPosition = false;
    }
  }

  if(validPosition)
  {
    m_data->coord_angle = iterAngle.GetPos();
    m_data->coord_y = iterY.GetPos();
  }

  return validPosition;
}