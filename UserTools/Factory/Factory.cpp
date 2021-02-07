#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="StepperMotorArduino") ret=new StepperMotorArduino;
if (tool=="StepperMotorStanda") ret=new StepperMotorStanda;
return ret;
}
