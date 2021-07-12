#include "Scheduler.h"

Scheduler::Scheduler():Tool()
{

}

bool Scheduler::Initialise(std::string configfile, DataModel &data)
{
  m_data = &data;
  m_log = m_data->Log;

  stateName[state::idle]               = "idle";
  stateName[state::init]               = "init";
  stateName[state::move]               = "move";  
  stateName[state::move_lin]           = "move_lin";
  stateName[state::move_rot]           = "move_rot";  
  stateName[state::record]             = "record";
  stateName[state::record_single_init] = "record_single_init";
  stateName[state::record_single]      = "record_single";
  stateName[state::finalise]           = "finalise";
  stateName[state::end]                = "end";

  useGUI = false;
  m_data->vars.Get("useGUI", useGUI);

  std::string socket_send;
  std::string socket_recv;
  m_data->vars.Get("zmqsocket_send", socket_send);
  m_data->vars.Get("zmqsocket_recv", socket_recv);

  context = new zmq::context_t(1);
  zmqsocket_send = new zmq::socket_t(*context, ZMQ_PUSH);
  zmqsocket_send->connect(socket_send);
  zmqsocket_recv = new zmq::socket_t(*context, ZMQ_SUB);
  zmqsocket_recv->connect(socket_recv);
  zmqsocket_recv->setsockopt(ZMQ_SUBSCRIBE, "", 0);

  if(useGUI)
  {
    Log("Scheduler: Waiting to receive initialise config from GUI", 1, 1);
    zmq::message_t config_msg;
    std::string config_str;
    zmqsocket_recv->recv(&config_msg, 0);
    config_str = (char*)config_msg.data();
    m_data->vars.JsonParser(config_str);
    Log("Scheduler: Got initialise config from GUI!", 1, 1);
  }
  else
  {
    file = new std::ifstream(configfile.c_str());
    std::string config_str((std::istreambuf_iterator<char>(*file)),
                           std::istreambuf_iterator<char>());
    m_data->vars.JsonParser(config_str);
    Log("Scheduler: Config loaded from file " + configfile, 1, m_verbose);
    file->close();
  }

  //-------------------------------------------------------------------------
  // Fix since Python Tools can't get std::string variables from ASCII stores
  //-------------------------------------------------------------------------
  std::string funcgen_IP;
  m_data->vars.Get("funcgen_IP", funcgen_IP);
  std::string delim = ".";
  int IPnum;
  for(int i=0; i<4; ++i)
  {
    IPnum = std::stoi(funcgen_IP.substr(0,funcgen_IP.find(delim)));
    funcgen_IP.erase(0, funcgen_IP.find(delim) + delim.length());
    m_data->vars.Set("funcgen_IP"+std::to_string(i), IPnum);
  }
  //-------------------------------------------------------------------------

  m_data->mode = state::idle;
  m_data->vars.Set("state", m_data->mode);

  return true;
}

bool Scheduler::Execute()
{
  switch (m_data->mode)
  {
    case state::idle:
    {
      if(useGUI)
      {
        zmq::message_t config_msg;
        std::string config_str;
        Log("Scheduler: Waiting to receive config from GUI", 1, 1);
        zmqsocket_recv->recv(&config_msg, 0);
        config_str = (char*)config_msg.data();
        m_data->tempstore.JsonParser(config_str);

        double coord;
        if(m_data->tempstore.Get("move_linmotor", coord))
        {
          Log("Scheduler: Received linear motor move command from GUI", 1, m_verbose);
          m_data->mode = state::move_lin;
          m_data->vars.Set("state", m_data->mode);
          m_data->coord_y = coord;
          break;
        }
        else if(m_data->tempstore.Get("move_rotmotor", coord))
        {
          Log("Scheduler: Received angular motor move command from GUI", 1, m_verbose);
          m_data->mode = state::move_rot;
          m_data->vars.Set("state", m_data->mode);
          m_data->coord_angle = coord;
          break;
        }
        else if(m_data->tempstore.Has("record_single"))
        {
          Log("Scheduler: Received record command from GUI", 1, m_verbose);
          m_data->mode = state::record_single_init;
          m_data->vars.Set("state", m_data->mode);
          break;
        }
        else
        {
          Log("Scheduler: Received config from GUI!", 1, m_verbose);
          m_data->vars.JsonParser(config_str);
        }
        m_data->tempstore.Delete();
      }

      m_data->mode = state::init;
      m_data->vars.Set("state", m_data->mode);

      std::string angleRangesString;
      std::string yRangesString;
      double stepSizeAngle;
      double stepSizeY;
      if(!m_data->vars.Get("verbose", m_verbose)) m_verbose = 1;
      m_data->vars.Get("stepSizeAngle", stepSizeAngle);
      m_data->vars.Get("stepSizeY", stepSizeY);
      m_data->vars.Get("rangesAngle", angleRangesString);
      m_data->vars.Get("rangesY", yRangesString);

      std::vector<std::tuple<double,double>> angleRanges = ParseRanges(angleRangesString);
      std::vector<std::tuple<double,double>> yRanges = ParseRanges(yRangesString);

      if(angleRanges.size() == 0 || yRanges.size() == 0)
      {
        Log("Scheduler: No scan ranges found. Exiting.", 1, m_verbose);
        m_data->mode = state::end;
        m_data->vars.Set("state", m_data->mode);
        m_data->vars.Set("StopLoop",1);
        break;
      }

      m_iterAngle.Initialise(stepSizeAngle, angleRanges);
      m_iterY.Initialise(stepSizeY, yRanges);
      break;
    }

    case state::init:
    {
      m_data->mode = state::move;
      m_data->vars.Set("state", m_data->mode);

      m_data->coord_angle = m_iterAngle.GetPos();
      m_data->coord_y = m_iterY.GetPos();
      break;
    }

    case state::move_lin:
    {
      m_data->mode = state::idle;
      m_data->vars.Set("state", m_data->mode);
      break;
    }

    case state::move_rot:
    {
      m_data->mode = state::idle;
      m_data->vars.Set("state", m_data->mode);
      break;
    }

    case state::move:
    {
      m_data->mode = state::record;
      m_data->vars.Set("state", m_data->mode);
      break;
    }

    case state::record:
    {
      m_data->mode = state::move;
      m_data->vars.Set("state", m_data->mode);

      zmq::message_t msg = ZMQCreateWaveformMessage("multi", m_data->coord_angle, m_data->coord_y, m_data->waveform_PMT, m_data->waveform_PD);
      zmqsocket_send->send(msg);

      if(!UpdateMotorCoords())
        m_data->mode = state::finalise;

      break;
    }

    case state::record_single_init:
    {
      m_data->mode = state::record_single;
      m_data->vars.Set("state", m_data->mode);
      break;
    }

    case state::record_single:
    {
      m_data->mode = state::idle;
      m_data->vars.Set("state", m_data->mode);

      zmq::message_t msg = ZMQCreateWaveformMessage("single", m_data->coord_angle, m_data->coord_y, m_data->waveform_PMT, m_data->waveform_PD);
      zmqsocket_send->send(msg);

      break;
    }

    case state::finalise:
    {
      if(useGUI)
      {
        m_data->mode = state::idle;
        m_data->vars.Set("state", m_data->mode);
      }
      else
      {
        m_data->vars.Set("StopLoop",1);
        m_data->mode = state::end;
        m_data->vars.Set("state", m_data->mode);
      }
      break;
    }
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
  if(!m_iterAngle.NextPos())
  {
    if(!m_iterY.NextPos())
    {
      validPosition = false;
    }
  }

  if(validPosition)
  {
    m_data->coord_angle = m_iterAngle.GetPos();
    m_data->coord_y = m_iterY.GetPos();
  }

  return validPosition;
}

zmq::message_t Scheduler::ZMQCreateWaveformMessage(std::string mode, double angle, double ypos, std::vector<double> waveform_PMT, std::vector<double> waveform_PD)
{
  std::tuple<std::string, double, double, std::vector<double>, std::vector<double>> msgtuple(mode, angle, ypos, waveform_PMT, waveform_PD);
  msgpack::sbuffer msgtuple_packed;
  msgpack::pack(&msgtuple_packed, msgtuple);
  zmq::message_t message(msgtuple_packed.size());
  std::memcpy(message.data(), msgtuple_packed.data(), msgtuple_packed.size());
  return message;
}
