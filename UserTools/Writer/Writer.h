#ifndef Writer_H
#define Writer_H

#include <string>
#include <iostream>

#include <cstdlib> // Delete later
#include <ctime>   // Delete later
#include <cmath>   // Delete later
#define _USE_MATH_DEFINES // Delete later

#include "Tool.h"
#include "TTree.h"
//#include "DfTree.h"


/**
 * \class Writer
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class Writer: public Tool {


 public:

  Writer(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  bool WriteFile();

  int m_verbose;
  std::string m_fileName;
  std::string m_treeName;

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

  // Necessary pointers
  std::vector<double> *p_waveform_PMT;
  std::vector<double> *p_waveform_PD;

  //////////////////////
  //////////////////////


 private:

  TFile *file;
  TTree *tree;


};


#endif
