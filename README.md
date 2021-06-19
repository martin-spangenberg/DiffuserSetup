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

### Running the code:
To start the program, simply execute
```
./run.sh
```
