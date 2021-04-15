#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="StepperMotorArduino") ret=new StepperMotorArduino;
if (tool=="StepperMotorStanda") ret=new StepperMotorStanda;
if (tool=="FunctionGenerator") ret=new FunctionGenerator;
if (tool=="Scope") ret=new Scope;
if (tool=="Writer") ret=new Writer;
if (tool=="Scheduler") ret=new Scheduler;
if (tool=="HumidityReader") ret=new HumidityReader;
if (tool=="FunctionGeneratorDummy") ret=new FunctionGeneratorDummy;
if (tool=="ScopeDummy") ret=new ScopeDummy;
if (tool=="HumidityReaderDummy") ret=new HumidityReaderDummy;
if (tool=="StepperMotorArduinoDummy") ret=new StepperMotorArduinoDummy;
if (tool=="StepperMotorStandaDummy") ret=new StepperMotorStandaDummy;

return ret;
}
