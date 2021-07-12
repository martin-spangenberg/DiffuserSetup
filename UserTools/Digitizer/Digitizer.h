#ifndef Digitizer_H
#define Digitizer_H

#include <string>
#include <iostream>
#include <math.h>

#include "Tool.h"

#include <c_header/dlltyp.h>
#include <c_header/regs.h>
#include <c_header/spcerr.h>
#include <c_header/spcm_drv.h>
#include <common/ostools/spcm_oswrap.h>
#include <common/ostools/spcm_ostools.h>

#include <chrono>
using namespace std::chrono;

/**
* \class Digitizer
*
* Author: Martin Spangenberg
* Contact: m.spangenberg.1@warwick.ac.uk
*/
class Digitizer: public Tool
{

 public:

  Digitizer(); ///< Simple constructor
  bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  bool StartDMA();
  bool StopDMA();
  bool RecordWaveform();

 private:

  // Input variables
  int32 m_numSamples; // Number of samples to keep per trigger event
  int32 m_numAverages; // Number of trigger events to average over
  int32 m_triggerLevel; // Trigger level [mV]
  int32 m_inputRange; // Input range [mV]. Only a few present available: {2500, 1000, 500, 200}
  int32 m_inputOffsetPercent; // Translates the input range by this many percent up to +/- 200%
  int64 m_sampleRate; // Sampling rate in Hz

  drv_handle  cardHandle;
  //int32       cardType, cardSerial, lFncType;
  int8*       dataBlock;
  std::vector<double> *averagePMT;
  std::vector<double> *averagePD;
  char        szErrorTextBuffer[ERRORTEXTLEN];
  uint32      dwError;
  int32       lStatus;
  int64       llAvailUser, llPCPos, mempos;
  int32       lFillsize;

  // FIFO mode buffer handling
  int32       lNotifySize;
  int64       llBufferSize;
  int32       lSegmentCount;
  int32       lAverageCount; 

  char* szTypeToName(int32 lCardType)
  {
    static char szName[50];
    switch (lCardType & TYP_SERIESMASK)
    {
      case TYP_M2ISERIES:     sprintf (szName, "M2i.%04x", lCardType & TYP_VERSIONMASK);      break;
      case TYP_M2IEXPSERIES:  sprintf (szName, "M2i.%04x-Exp", lCardType & TYP_VERSIONMASK);  break;
      case TYP_M3ISERIES:     sprintf (szName, "M3i.%04x", lCardType & TYP_VERSIONMASK);      break;
      case TYP_M3IEXPSERIES:  sprintf (szName, "M3i.%04x-Exp", lCardType & TYP_VERSIONMASK);  break;
      case TYP_M4IEXPSERIES:  sprintf (szName, "M4i.%04x-x8", lCardType & TYP_VERSIONMASK);   break;
      case TYP_M4XEXPSERIES:  sprintf (szName, "M4x.%04x-x4", lCardType & TYP_VERSIONMASK);   break;
      case TYP_M2PEXPSERIES:  sprintf (szName, "M2p.%04x-x4", lCardType & TYP_VERSIONMASK);   break;
      default:                sprintf (szName, "unknown type");                               break;
    }
  return szName;
  }

};


#endif
