#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="HumidityReaderDummy") ret=new HumidityReaderDummy;
if (tool=="HumidityReader") ret=new HumidityReader;
if (tool=="PythonScript") ret=new PythonScript;
if (tool=="Scheduler") ret=new Scheduler;
if (tool=="StepperMotorArduinoDummy") ret=new StepperMotorArduinoDummy;
if (tool=="StepperMotorArduino") ret=new StepperMotorArduino;
if (tool=="StepperMotorStandaDummy") ret=new StepperMotorStandaDummy;
if (tool=="StepperMotorStanda") ret=new StepperMotorStanda;
if (tool=="Writer") ret=new Writer;
  if (tool=="Digitizer") ret=new Digitizer;
return ret;
}
