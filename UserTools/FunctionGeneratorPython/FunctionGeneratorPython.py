#### my python value Generator
import Store
import pyvisa
import time
import sys

def Initialise(var):

    global m_channel; m_channel = 1
    global m_cycles; m_cycles = 0
    global m_frequency; m_frequency = 0
    global m_Vmin; m_Vmin = 0
    global m_Vmax; m_Vmax = 0
    global m_shape; m_shape = ""
    global m_IP; m_IP = ""
    for i in range(4):
       m_IP += str(Store.GetStoreVariable("vars", "funcgen_IP"+str(i), "int"))
       if i < 3:
          m_IP += "."
    global visaman;     visaman     = pyvisa.ResourceManager('@py')

    try:
       global m_instrument; m_instrument = visaman.open_resource("TCPIP::"+m_IP+"::INSTR")
       print("FunctionGeneratorPython: Connected to device on IP "+m_IP)
    except:
       print("FunctionGeneratorPython: Could not connect to instrument at IP "+m_IP+"!!")
       return 0

    InitSetup()

    return 1

def Finalise():
    return 1

def Execute():

    state = Store.GetStoreVariable("vars", "state", "int")

    if state == 1: # init
        SetAllVariables()

    elif state == 5: # record
        SendTrigger()

    elif state == 6: # record_single_init
        SetAllVariables()

    elif state == 7: # record_single
        SendTrigger()

    return 1

def InitSetup():
    print("FunctionGeneratorPython: Initial setup - trigger mode: EXTERNAL")
    m_instrument.write("TRIGGER:SEQUENCE:SOURCE EXTERNAL")

def ActivateChannel(channel):
    global m_channel
    if channel in range(1,3):
        print("FunctionGeneratorPython: Setting output channel to "+str(channel))
        #m_instrument.write("OUTPUT"+str(m_channel)+":STATE OFF")
        m_channel = channel
        m_instrument.write("OUTPUT"+str(channel)+":STATE ON")
        m_instrument.write("SOURCE"+str(channel)+":BURST:STATE ON")
    else:
        print("FunctionGeneratorPython: Invalid channel number "+str(channel)+" selected")

def SetShape(shape):
    global m_shape
    print("FunctionGeneratorPython: Setting function shape to "+shape)
    m_shape = shape
    m_instrument.write("SOURCE"+str(m_channel)+":FUNCTION:SHAPE "+shape)

def SetNCycles(cycles):
    global m_cycles 
    print("FunctionGeneratorPython: Setting cycles per burst to "+str(cycles))
    m_cycles = cycles
    m_instrument.write("SOURCE"+str(m_channel)+":BURST:NCYCLES "+str(cycles))

def SetFrequencyHz(frequency):
    global m_frequency
    print("FunctionGeneratorPython: Setting function frequency to "+str(frequency)+" Hz")
    m_frequency = frequency
    m_instrument.write("SOURCE"+str(m_channel)+":FREQUENCY "+str(m_frequency))

def SetOutputVoltsMin(Vmin):
    global m_Vmin
    print("FunctionGeneratorPython: Setting function minimum to "+str(Vmin)+" Volts")
    m_Vmin = Vmin
    m_instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:LOW "+str(Vmin)+"V")

def SetOutputVoltsMax(Vmax):
    global m_Vmax
    print("FunctionGeneratorPython: Setting function maximum to "+str(Vmax)+" Volts")
    m_Vmax = Vmax
    m_instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:HIGH "+str(Vmax)+"V")

def SetAllVariables():
    channel   = Store.GetStoreVariable("vars", "funcgen_channel", "int")
    cycles    = Store.GetStoreVariable("vars", "funcgen_cycles", "int")
    frequency = Store.GetStoreVariable("vars", "funcgen_frequency", "double")
    Vmin      = Store.GetStoreVariable("vars", "funcgen_Vmin", "double")
    Vmax      = Store.GetStoreVariable("vars", "funcgen_Vmax", "double")
    shape = "SQUARE"

    ActivateChannel(channel)
    SetShape(shape)
    SetNCycles(cycles)
    SetFrequencyHz(frequency)
    SetOutputVoltsMin(Vmin)
    SetOutputVoltsMax(Vmax)

def SendTrigger():
    print("FunctionGeneratorPython: Sending trigger")
    m_instrument.write("*TRG")
