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
  m_variables.Get("stepSizeAngle",stepSizeAngle);
  m_variables.Get("stepSizeY",stepSizeY);
  m_variables.Get("angleRanges",angleRangesString);
  m_variables.Get("yRanges",yRangesString);

  std::vector<std::tuple<double,double>> angleRanges = ParseRanges(angleRangesString);
  std::vector<std::tuple<double,double>> yRanges = ParseRanges(yRangesString);

  iterAngle.Initialise(stepSizeAngle, angleRanges);
  iterY.Initialise(stepSizeY, yRanges);

  m_data->coord_angle = iterAngle.GetPos();
  m_data->coord_y = iterY.GetPos();
  m_data->mode = state::idle;

  stateName[state::idle]     = "idle";
  stateName[state::init]     = "init";
  stateName[state::move]     = "move";
  stateName[state::record]   = "record";
  stateName[state::finalise] = "finalise";



  return true;
}

bool Scheduler::Execute()
{

  Log("Scheduler: Starting mode "+stateName[m_data->mode], 1, m_verbose);

  std::cout << "(y, angle) = (" << m_data->coord_y << ", " << m_data->coord_angle << ")" << std::endl;

  switch (m_data->mode)
  {
    case state::idle:
      m_data->mode = state::init;
      break;

    case state::init:
      m_data->mode = state::move;
      break;

    case state::move:
      m_data->mode = state::record;
      break;

    case state::record:
      if(!iterAngle.NextPos())
      {
        if(!iterY.NextPos())
        {
          m_data->mode = state::finalise;
          break;
        }
      }
      m_data->coord_angle = iterAngle.GetPos();
      m_data->coord_y = iterY.GetPos();
      m_data->mode = state::move;
      break;

    case state::finalise:
      m_data->vars.Set("StopLoop",1);
      break;
  }

  return true;
}

bool Scheduler::Finalise()
{

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

// bool Scheduler::NextCoordinates()
// {
//   bool finished = false;
//   double new_angle = m_data->coord_angle + m_stepSizeAngle;
//   if(new_angle > std::get<1>(m_angleRanges.at(curr_angleRange)))
//   {
//     curr_angleRange += 1;
//     if (curr_angleRange > m_angleRanges.size()-1)
//     {
//       curr_angleRange = 0;
//       curr_yRange += 1;
//       m_data->coord_y = std::get<0>(m_yRanges.at(curr_yRange));
//     }

//     m_data->coord_angle = std::get<0>(m_angleRanges.at(curr_angleRange));
//     m_data->mode = state::move;
//         }
//         else
          
//   }
//     else
//     {
//       m_data->coord_angle = new_angle;
//     }
//   }
//   return finished;
// }

// bool Scheduler::NextCoord(double stepsize, std::vector<std::tuple<double,double>> ranges)
// {
//   bool finished = false;
//   double new_coord = ranges
// }