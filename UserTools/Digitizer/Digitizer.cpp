#include "Digitizer.h"

Digitizer::Digitizer():Tool(){}


bool Digitizer::Initialise(std::string configfile, DataModel &data)
{
  m_data= &data;
  m_log= m_data->Log;

  if(!m_data->vars.Get("verbose", m_verbose)) m_verbose = 1;
  m_data->vars.Get("digitizer_numSamples", m_numSamples);
  m_data->vars.Get("funcgen_cycles", m_numAverages);
  m_data->vars.Get("digitizer_triggerLevel", m_triggerLevel);
  m_data->vars.Get("digitizer_inputRange", m_inputRange);
  m_data->vars.Get("digitizer_inputOffsetPercent", m_inputOffsetPercent);
  m_data->vars.Get("digitizer_sampleRate", m_sampleRate);

  //m_numSamples = 10000; // Number of samples to keep per trigger event
  //m_numAverages = 1000; // Number of trigger events to average over
  //m_triggerLevel = 100; // Trigger level [mV]
  //m_inputRange = 2500; // Input range [mV]
  //m_inputOffsetPercent = -95;
  //m_sampleRate = 2.5*pow(10,9);

  // FIFO mode buffer handling
  //lNotifySize = 4096 * ceil(m_numSamples * 2 / 4096.); // Can only collect samples in multiples of 4096
  lNotifySize = 4096 * ceil(m_numSamples / 4096.); // Can only collect samples in multiples of 4096

  llBufferSize = lNotifySize * (m_numAverages + 2); // software (DMA) buffer size
  lAverageCount = 0; // Number of averages

  // Open card
  cardHandle = spcm_hOpen("/dev/spcm0");
  if(!cardHandle)
  {
    Log("Digitizer: No card found!", 1, m_verbose);
    return false;
  }

  // Define the data buffers
  dataBlock = (int8*)pvAllocMemPageAligned((uint64)llBufferSize);
  averagePMT = new std::vector<double>(m_numSamples, 0);
  //averagePD  = new std::vector<double>(m_numSamples, 0);
  //if(!dataBlock || !averagePMT || !averagePD)
  if(!dataBlock)  
  {
    Log("Digitizer: Memory allocation failed!", 1, m_verbose);
    spcm_vClose(cardHandle);
    return false;
  }

  spcm_dwSetParam_i32(cardHandle, SPC_CHENABLE,         CHANNEL0);              // Just 1 channel enabled
  //spcm_dwSetParam_i32(cardHandle, SPC_CHENABLE,         CHANNEL0 | CHANNEL1);   // Channel 1 and 2 enabled  

  // Channel 0
  spcm_dwSetParam_i32(cardHandle, SPC_AMP0,             m_inputRange);          // Max value in symmetric input range [mV]
  spcm_dwSetParam_i32(cardHandle, SPC_OFFS0,            -m_inputOffsetPercent); // Percent input range offset
  spcm_dwSetParam_i32(cardHandle, SPC_ACDC0,            0);                     // Set DC coupling

  // Channel 1
  //spcm_dwSetParam_i32(cardHandle, SPC_AMP1,             m_inputRange);          // Max value in symmetric input range [mV]
  //spcm_dwSetParam_i32(cardHandle, SPC_OFFS1,            -m_inputOffsetPercent); // Percent input range offset
  //spcm_dwSetParam_i32(cardHandle, SPC_ACDC1,            0);                     // Set DC coupling

  spcm_dwSetParam_i32(cardHandle, SPC_CARDMODE,         SPC_REC_FIFO_MULTI);    // Multiple recording FIFO mode
  spcm_dwSetParam_i32(cardHandle, SPC_LOOPS,            0);                     // Endless
  spcm_dwSetParam_i32(cardHandle, SPC_SEGMENTSIZE,      lNotifySize);          // 1k of pretrigger data at start of FIFO mode
  spcm_dwSetParam_i32(cardHandle, SPC_POSTTRIGGER,      lNotifySize - 32);     // 32 samples pretrigger data for each segment
  spcm_dwSetParam_i32(cardHandle, SPC_TIMEOUT,          0);                     // No timeout
  spcm_dwSetParam_i32(cardHandle, SPC_TRIG_ORMASK,      SPC_TMASK_EXT0);        // Trigger set to external input Ext0
  spcm_dwSetParam_i32(cardHandle, SPC_TRIG_ANDMASK,     0);                     // ...
  spcm_dwSetParam_i32(cardHandle, SPC_CLOCKMODE,        SPC_CM_INTPLL);         // Clock mode internal PLL
  spcm_dwSetParam_i64(cardHandle, SPC_SAMPLERATE,       m_sampleRate);          // Sampling rate in HZ
  spcm_dwSetParam_i32(cardHandle, SPC_CLOCKOUT,         0);                     // No clock output
  spcm_dwSetParam_i32(cardHandle, SPC_TRIG_EXT0_MODE,   SPC_TM_POS);            // Rising edge trigger
  spcm_dwSetParam_i32(cardHandle, SPC_TRIG_EXT0_LEVEL0, m_triggerLevel);        // External trigger level

  // Define transfer
  spcm_dwDefTransfer_i64(cardHandle, SPCM_BUF_DATA, SPCM_DIR_CARDTOPC, lNotifySize, dataBlock, 0, llBufferSize);

  // Start everything
  dwError = spcm_dwSetParam_i32(cardHandle, SPC_M2CMD, M2CMD_CARD_START | M2CMD_CARD_ENABLETRIGGER | M2CMD_DATA_STARTDMA);

  // Check for errors
  if (dwError != ERR_OK)
  {
    spcm_dwGetErrorInfo_i32(cardHandle, NULL, NULL, szErrorTextBuffer);
    Log("Digitizer: Error message: "+std::string(szErrorTextBuffer), 1, m_verbose);
    vFreeMemPageAligned(dataBlock, (uint64)llBufferSize);
    spcm_vClose(cardHandle);
    return false;
  }

  return true;
}

bool Digitizer::Execute()
{
  switch(m_data->mode)
  {
    case state::record:
    {
      auto start = high_resolution_clock::now();

      // Reset average vectors and count
      std::fill(averagePMT->begin(), averagePMT->end(), 0);
      //std::fill(averagePD->begin(), averagePD->end(), 0);
      lAverageCount = 0;

      //for(int k=0; k<m_numAverages; ++k)
      //{
      //  spcm_dwSetParam_i32(cardHandle, SPC_M2CMD, M2CMD_CARD_FORCETRIGGER);
      //  usleep(1);
      //}

      // run the FIFO mode and loop through the data
      while(true)
      {
        // Wait for interrupt and check if we have new data
        if((dwError = spcm_dwSetParam_i32 (cardHandle, SPC_M2CMD, M2CMD_DATA_WAITDMA)) == ERR_OK)
        {
          spcm_dwGetParam_i32(cardHandle, SPC_M2STATUS,             &lStatus);
          spcm_dwGetParam_i64(cardHandle, SPC_DATA_AVAIL_USER_LEN,  &llAvailUser);
          spcm_dwGetParam_i64(cardHandle, SPC_DATA_AVAIL_USER_POS,  &llPCPos);

          if(lStatus & M2STAT_DATA_OVERRUN)
          {
            Log("Digitizer: Buffer overrun!", 1, m_verbose);
            break;
          }

          if(llAvailUser >= lNotifySize)
          {
            lAverageCount += 1;

            // Add latest waveforms to average data
            for(int i=0; i<m_numSamples; ++i)
            {
              averagePMT->at(i) += (double)dataBlock[llPCPos + i];
              //averagePMT->at(i) += (double)dataBlock[llPCPos + 2*i];
              //averagePD->at(i)  += (double)dataBlock[llPCPos + 2*i + 1];
            }

            // free the buffer
            spcm_dwSetParam_i32(cardHandle, SPC_DATA_AVAIL_CARD_LEN, lNotifySize);

            if(lAverageCount >= m_numAverages)
            {
              Log("Digitizer: Got all "+std::to_string(m_numAverages)+" triggers", 1, m_verbose);
              break;
            }
          }      
        }
        
        // Something went wrong
        else
        {
          if(dwError == ERR_TIMEOUT)
            Log("Digitizer: Timeout!", 1, m_verbose);
          else
          {
            Log("Digitizer: Error: "+std::to_string(dwError), 1, m_verbose);
            break;
          }
        }
      }

      double scalefactor = m_inputRange * 0.001 / (127. * m_numAverages);
      double offset = m_inputRange * 0.001 * m_inputOffsetPercent * 0.01;

      for(int i=0; i<m_numSamples; ++i)
      {
        averagePMT->at(i) = averagePMT->at(i) * scalefactor + offset;
        //averagePD->at(i) = averagePD->at(i) * scalefactor + offset;      
      }

      m_data->waveform_PMT = *averagePMT;
      //m_data->waveform_PD  = *averagePD;

      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<microseconds>(stop - start);
      std::cout << "Time taken: " << duration.count()/1000. << " milliseconds" << std::endl;

    }
  }

  return true;
}

bool Digitizer::Finalise()
{
  // Send card stop command
  dwError = spcm_dwSetParam_i32(cardHandle, SPC_M2CMD, M2CMD_CARD_STOP | M2CMD_DATA_STOPDMA);

  // Clean up memory
  Log("Digitizer: Finalising", 1, m_verbose);
  vFreeMemPageAligned(dataBlock, (uint64)llBufferSize);
  averagePMT->clear();
  //averagePD->clear();
  spcm_vClose(cardHandle);

  return true;
}

