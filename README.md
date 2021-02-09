# DiffuserSetup

This software controls the HyperK diffuser DAQ system. It is based on the ToolDAQ framework[1].

****************************
#Setup
****************************

After cloning this repository, run the command ./GetToolDAQ.sh

This fetches the main ToolDAQ framework and compiles it. Compilation requires the g++ compiler to be installed on your system.

The Tool StepperMotorStanda depends on the libximc library: http://files.xisupport.com/libximc/libximc-2.12.5-all.tar.gz

Once downloaded, find the subfolder in ximc/ relevant for your distro and install the main library (starts with libximc7) and the developer additions (libximc7-dev...). 

For more information on ToolDAQ, consult the ToolDAQ documentation:

https://github.com/ToolDAQ/ToolDAQFramework/blob/master/ToolDAQ%20doc.pdf

[1] Benjamin Richards. (2018, November 11). ToolDAQ Framework v2.1.1 (Version V2.1.1). Zenodo. http://doi.org/10.5281/zenodo.1482767 
