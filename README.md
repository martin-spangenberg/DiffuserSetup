# DiffuserSetup
This software controls the WATCHMAN diffuser DAQ system. It is based on the ToolDAQ framework[1]. The setup has been tested to work on Ubuntu 20.04. Other distributions will require slightly different setup.

## Setup on Ubuntu 20.04

### Install Spectrum ADC drivers:
Install the PCIe card on the motherboard. Then get the driver source code from the DVD (not available online).
```
tar -xvzf spcm4_linux_drvsource_2.17-17916.tgz
cd spcm4_linuxkernel_source
sudo chmod -x make_spcm4_linux_kerneldrv.sh
sudo ./make_spcm4_linux_kerneldrv.sh
```

### Install libximc:
```
wget http://files.xisupport.com/libximc/libximc-2.12.5-all.tar.gz
tar -xvzf libximc-2.12.5-all.tar.gz
sudo dpkg -i ximc-2.12.5/ximc/deb/libximc7_2.12.5-1_amd64.deb
sudo dpkg -i ximc-2.12.5/ximc/deb/libximc7-dev_2.12.5-1_amd64.deb
```

### Python dependencies:
Install python3.6 (newer versions currently don’t work with the ToolDAQ code):
```
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt -y install python3.6 python3.6-dev
sudo update-alternatives --install /usr/local/bin/python3 python3 /usr/bin/python3.6 20
```
Update pip and install packages:
```
python3 -m pip install --user --upgrade pip
python3 -m pip install --user -f https://extras.wxpython.org/wxPython4/extras/linux/gtk3/ubuntu-20.04 wxPython
python3 -m pip install --user matplotlib zmq msgpack pyvisa pyvisa-py
```
The Python GUI also needs the SDL library:
```
sudo apt -y install libsdl2-dev
```

### Compile DiffuserSetup code:
```
sudo apt -y install git gcc g++ make cmake binutils libx11-dev libxpm-dev libxft-dev libxext-dev libssl-dev
git clone http://github.com/martin-spangenberg/DiffuserSetup.git
cd DiffuserSetup
./GetToolDAQ.sh
```
The last command fetches the ToolDAQ framework and supporting libraries and compiles them in the subfolder “ToolDAQ”. It also compiles the diffuser DAQ program. If it needs to be recompiled after changes, simply do:
```
make clean
make
```

### Running the DAQ program:
To run the program, simply execute:
```
source Setup.sh
./run.sh
```

## Notes

Python API bindings have changed in newer versions, hence Python 3.6 needs to be installed in order to compile and run the DAQ program. If the user wants to import the ROOT module in Python then version 3.8 must be used instead, since the ROOT 6 binary has been compiled for this version. Python 3.8 is installed by default on Ubuntu 20.04.

### Driver details
The PMT vertical position is adjusted with a linear stepper motor controlled through a dedicated USB-connected controller from Standa. It requires the libximc library.

The Tektronix function generator triggering the laser and digitizer is controlled with VISA serial commands over TCP/IP. The code previously used the National Instruments VISA drivers (NI-VISA). Unfortunately, the source code for these kernel drivers is not available, and hence compatibility is limited to a few specific Linux kernel versions and distributions for which National Instruments have provided pre-compiled drivers. NI-VISA was therefore dropped in favour of PyVISA with a pure Python backend, which can be installed easily with pip commands.

Kernel drivers are required in order to communicate with the Spectrum PCIe digitizer card. Pre-compiled drivers are available for a limited number of kernel versions. Spectrum has provided us with a copy of the driver source code but we are not allowed to distribute it, so it is not included in this repository. A copy has been physically placed alongside the manual and other software for the digitizer. The source code does not compile on CentOS, but Ubuntu 20.04 and OpenSUSE Leap 15.x are compatible.
