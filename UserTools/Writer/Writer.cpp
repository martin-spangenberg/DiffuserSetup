#include "Writer.h"

Writer::Writer():Tool()
{

}

bool Writer::Initialise(std::string configfile, DataModel &data)
{
  //////////////////////////
  // Output format version//
  //////////////////////////
  version_major = 1; // Major version
  version_minor = 0; // Minor version
  version_patch = 0; // Patch version
  // Version numbers should be incremented in case changes are made to the output file format
  // They should follow semantic versioning as described on semver.org

  m_data = &data;
  m_log = m_data->Log;

  if(!m_data->vars.Get("verbose", m_verbose)) m_verbose = 1;

  p_waveform_PMT = &waveform_PMT;
  p_waveform_PD = &waveform_PD;

  return true;
}

bool Writer::Execute()
{
  switch(m_data->mode)
  {
    case state::init:
    {
      if(!m_data->vars.Get("rootfilename", m_fileName)) m_fileName = "diffuser.root";
      if(!m_data->vars.Get("treename", m_treeName)) m_treeName = "diffuser";
      ID_diffuser = *m_data->vars["ID_diffuser"];
      ID_PMT = *m_data->vars["ID_PMT"];
      ID_PD = *m_data->vars["ID_PD"];
      ID_lightsource = *m_data->vars["ID_lightsource"];
      ID_experimentalist = *m_data->vars["ID_experimentalist"];
      notes = *m_data->vars["notes"];

      file = new TFile(m_fileName.c_str(), "RECREATE");
      tree = new TTree(m_treeName.c_str(),m_treeName.c_str());

      // Setup tree branches
      tree->Branch("dt", &dt);
      tree->Branch("pulse_rate", &pulse_rate);
      tree->Branch("pulse_N", &pulse_N);
      tree->Branch("ID_diffuser", &ID_diffuser);
      tree->Branch("ID_PMT", &ID_PMT);
      tree->Branch("ID_PD", &ID_PD);
      tree->Branch("ID_lightsource", &ID_lightsource);
      tree->Branch("ID_experimentalist", &ID_experimentalist);
      tree->Branch("notes", &notes);
      tree->Branch("version_major", &version_major);
      tree->Branch("version_minor", &version_minor);
      tree->Branch("version_patch", &version_patch);
      tree->Branch("time_epoch", &time_epoch);
      tree->Branch("time_year", &time_year);
      tree->Branch("time_month", &time_month);
      tree->Branch("time_day", &time_day);
      tree->Branch("time_hour", &time_hour);
      tree->Branch("time_minute", &time_minute);
      tree->Branch("time_second", &time_second);
      tree->Branch("coord_angle", &coord_angle);
      tree->Branch("coord_y", &coord_y);
      tree->Branch("lab_temp", &lab_temp);
      tree->Branch("lab_humid", &lab_humid);
      tree->Branch("waveform_PMT", &p_waveform_PMT);
      tree->Branch("waveform_PD", &p_waveform_PD);
      break;
    }

    case state::record:
    {
      Log("Writer: Writing collected data", 1, m_verbose);
      WriteFile();
      break;
    }

    case state::finalise:
    {
      Log("Writer: Writing file to disk", 1, m_verbose);
      file->Write();
      file->Close();
      break;
    }
  }

  return true;
}

bool Writer::Finalise()
{

  return true;
}

bool Writer::WriteFile()
{
  long long sampleRate;
  m_data->vars.Get("digitizer_sampleRate", sampleRate);
  dt = 1./sampleRate;
  m_data->vars.Get("funcgen_frequency", pulse_rate);
  m_data->vars.Get("funcgen_cycles", pulse_N);

  std::time_t currtime = time(NULL);
  time_epoch = currtime;
  struct tm *timestruct = localtime(&currtime);
  time_year = timestruct->tm_year + 1900;
  time_month = timestruct->tm_mon + 1;
  time_day = timestruct->tm_mday;
  time_hour = timestruct->tm_hour;
  time_minute = timestruct->tm_min;
  time_second = timestruct->tm_sec;
  coord_angle = m_data->coord_angle;
  coord_y = m_data->coord_y;
  lab_temp = m_data->lab_temp;
  lab_humid = m_data->lab_humid;

  waveform_PMT = m_data->waveform_PMT;
  waveform_PD = m_data->waveform_PD;

  tree->Fill();


  return true;
}