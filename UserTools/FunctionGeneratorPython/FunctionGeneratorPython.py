#### my python value Generator
import Store
import pyvisa
import time
import sys

def Initialise(var):

    print("Python version:")
    print(sys.version)

    global m_channel;   m_channel   = Store.GetStoreVariable("vars", "funcgen_channel", "int")
    #global m_shape;     m_shape     = Store.GetStoreVariable("vars", "funcgen_shape", "std::string")
    global m_shape; m_shape = "SQUARE"
    global m_cycles;    m_cycles    = Store.GetStoreVariable("vars", "funcgen_cycles", "int")
    global m_frequency; m_frequency = Store.GetStoreVariable("vars", "funcgen_frequency", "double")
    global m_Vmin;      m_Vmin      = Store.GetStoreVariable("vars", "funcgen_Vmin", "double")
    global m_Vmax;      m_Vmax      = Store.GetStoreVariable("vars", "funcgen_Vmax", "double")
    #global m_IP;        m_IP        = Store.GetStoreVariable("vars", "funcgen_IP", "std::string")
    global m_IP; m_IP = "137.205.188.34"
    global visaman;     visaman     = pyvisa.ResourceManager('@py')

    try:
       global m_instrument; m_instrument = visaman.open_resource("TCPIP::"+m_IP+"::INSTR")
    except:
       print("FunctionGeneratorPython: Could not connect to instrument at IP "+m_IP+"!!")
       return 0

    InitSetup()
    SetShape(m_shape)
    SetNCycles(m_cycles)
    SetFrequencyHz(m_frequency)
    SetOutputVoltsMin(m_Vmin)
    SetOutputVoltsMax(m_Vmax)
    ActivateChannel(m_channel)

    return 1

def Finalise():
    return 1

def Execute():

    state = Store.GetStoreVariable("vars", "state", "std::string")

    if state == "record":
        SendTrigger()
        #print("FunctionGeneratorPython: Sending trigger")
        #time.sleep(1)

    return 1

def InitSetup():
    print("FunctionGeneratorPython: Initial setup - output mode: BURST, trigger mode: EXTERNAL")
    m_instrument.write("SOURCE"+str(m_channel)+":BURST:STATE ON")
    m_instrument.write("TRIGGER:SEQUENCE:SOURCE EXTERNAL")

def ActivateChannel(channel):
    print("FunctionGeneratorPython: Setting output channel to "+str(channel))
    if channel in range(1,3):
        m_channel = channel
        m_instrument.write("OUTPUT"+str(channel)+":STATE ON")
    else:
        print("FunctionGeneratorPython: Invalid channel number "+str(channel)+" selected")

def SetShape(shape):
    print("FunctionGeneratorPython: Setting function shape to "+shape)
    m_shape = shape
    m_instrument.write("SOURCE"+str(m_channel)+":FUNCTION:SHAPE "+shape)

def SetNCycles(cycles):
    print("FunctionGeneratorPython: Setting cycles per burst to "+str(cycles))
    m_cycles = cycles
    m_instrument.write("SOURCE"+str(m_channel)+":BURST_CYCLES "+str(m_cycles))

def SetFrequencyHz(frequency):
    print("FunctionGeneratorPython: Setting function frequency to "+str(frequency)+" Hz")
    m_frequency = frequency
    m_instrument.write("SOURCE"+str(m_channel)+":FREQUENCY "+str(m_frequency))

def SetOutputVoltsMin(Vmin):
    print("FunctionGeneratorPython: Setting function minimum to "+str(Vmin)+" Volts")
    m_Vmin = Vmin
    m_instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:LOW "+str(Vmin)+"V")

def SetOutputVoltsMax(Vmax):
    print("FunctionGeneratorPython: Setting function maximum to "+str(Vmax)+" Volts")
    m_Vmax = Vmax
    m_instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:HIGH "+str(Vmax)+"V")

def SendTrigger():
    print("FunctionGeneratorPython: Sending trigger")
    m_instrument.write("*TRG")
