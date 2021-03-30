#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

#include "TTree.h"

#include "Store.h"
#include "BoostStore.h"
#include "Logging.h"
#include "Utilities.h"

#include "TFile.h"
//#include "DfTree.h"

#include <zmq.hpp>

enum state
{
  idle,
  init,
  move,
  record,
  finalise
};

class DataModel {


 public:
  
  DataModel(); ///< Simple constructor

  //DfTree* GetDfTree(std::string name);
  //void AddDfTree(std::string name, DfTree *tree);
  //void DeleteDfTree(std::string name);
  //int SizeDfTree();


  Store vars; ///< This Store can be used for any variables. It is an inefficent ascii based storage    
  BoostStore CStore; ///< This is a more efficent binary BoostStore that can be used to store a dynamic set of inter Tool variables.
  std::map<std::string,BoostStore*> Stores; ///< This is a map of named BooStore pointers which can be deffined to hold a nammed collection of any tipe of BoostStore. It is usefull to store data that needs subdividing into differnt stores.

  //std::map<std::string, DfTree*> m_dftrees;

  Logging *Log; ///< Log class pointer for use in Tools, it can be used to send messages which can have multiple error levels and destination end points  
  zmq::context_t* context; ///< ZMQ contex used for producing zmq sockets for inter thread,  process, or computer communication

  state mode; //state for scheduler

  //////////////////////
  // Output variables //
  //////////////////////

  // Constant for all scan points
  double dt;                       // Time step per waveform sample [s]
  double pulse_rate;               // Rate of laser trigger [Hz]
  int pulse_N;                     // Number of averaged pulses
  std::string ID_diffuser;         // ID of diffuser sample
  std::string ID_PMT;              // ID of photomultiplier used
  std::string ID_PD;               // ID of photodiode used
  std::string ID_lightsource;      // ID of light source used
  std::string ID_experimentalist;  // Name of person responsible for data
  std::string notes;               // Freeform notes
  int version_major;               // Major version of data format
  int version_minor;               // Minor version of data format
  int version_patch;               // Patch version of data format

  // Updated for each scan point
  ULong64_t time_epoch;            // Time of data collection as Unix epoch
  int time_year;
  int time_month;
  int time_day;
  int time_hour;
  int time_minute;
  int time_second;
  double coord_angle;               // Angle of PMT [degrees]
  double coord_y;                   // Y-coordinate of PMT [mm]
  double lab_temp;                  // Lab temperature [C]
  double lab_humid;                 // Lab humidity [%]
  std::vector<double> waveform_PMT; // Waveform from photomultiplier [V]
  std::vector<double> waveform_PD;  // Waveform from photodiode [V]

  //////////////////////
  //////////////////////

 private:
  
  
  
};



#endif
