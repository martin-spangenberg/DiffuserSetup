#### my python value Generator
import Store
import pyvisa
import time

def Initialise(var):
    global channel
    global shape
    global cycles
    global frequency
    global Vmin
    global Vmax
    global IPaddress
    global visaman
    global instrument

    m_channel = 1
    m_shape = "SQUARE"
    m_cycles = 100
    m_frequency = 2000
    m_Vmin = 0.
    m_Vmax = 5.
    m_IPaddress = 137.205.188.34

    # m_channel = Store.GetStoreVariable("CStore", "funcgen_channel")
    # m_shape = Store.GetStoreVariable("CStore", "funcgen_shape")
    # m_cycles = Store.GetStoreVariable("CStore", "funcgen_cycles")
    # m_frequency = Store.GetStoreVariable("CStore", "funcgen_frequency")    
    # m_Vmin = Store.GetStoreVariable("CStore", "funcgen_Vmin")
    # m_Vmax = Store.GetStoreVariable("CStore", "funcgen_Vmax")
    # m_IPaddress = Store.GetStoreVariable("CStore", "funcgen_IPaddress")

    visaman = pyvisa.ResourceManager('@py')

    try:
        instrument = visaman.open_resource("TCPIP::"+m_IPaddress+"::INSTR")
    except:
        print("FunctionGeneratorPython: Cold not connect to instrument at IP "+m_IPaddress+"!!")
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

    state = Store.GetStoreVariable("CStore", "state")

    if state == "record":
        SendTrigger()
        time.sleep(1)


    # Store.SetInt('a',6)
    # Store.SetDouble('b',8.0)
    # Store.SetString('c','hello')
    return 1

def InitSetup():
    print("FunctionGeneratorPython: Initial setup - output mode: BURST, trigger mode: EXTERNAL")
    instrument.write("SOURCE"+str(m_channel)+":BURST:STATE ON")
    instrument.write("TRIGGER:SEQUENCE:SOURCE EXTERNAL")

def ActivateChannel(channel):
    print("FunctionGeneratorPython: Setting output channel to "+str(channel))
    if channel in range(1,3):
        m_channel = channel
        instrument.write("OUTPUT"+str(channel)+":STATE ON")
    else:
        print("FunctionGeneratorPython: Invalid channel number "+str(channel)+" selected")

def SetShape(shape):
    print("FunctionGeneratorPython: Setting function shape to "+shape)
    m_shape = shape
    instrument.write("SOURCE"+str(m_channel)+":FUNCTION:SHAPE "+shape)

def SetNCycles(cycles):
    print("FunctionGeneratorPython: Setting cycles per burst to "+str(cycles))
    m_cycles = cycles
    instrument.write("SOURCE"+str(m_channel)+":BURST_CYCLES "+str(m_cycles))

def SetFrequencyHz(frequency):
    print("FunctionGeneratorPython: Setting function frequency to "+str(frequency)+" Hz")
    m_frequency = frequency
    instrument.write("SOURCE"+str(m_channel)+":FREQUENCY "+str(m_frequency))

def SetOutputVoltsMin(Vmin):
    print("FunctionGeneratorPython: Setting function minimum to "+str(Vmin)+" Volts")
    m_Vmin = Vmin
    instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:LOW "+str(Vmin)+"V")

def SetOutputVoltsMax(Vmin):
    print("FunctionGeneratorPython: Setting function maximum to "+str(Vmax)+" Volts")
    m_Vmax = Vmax
    instrument.write("SOURCE"+str(m_channel)+":VOLTAGE:LEVEL:IMMEDIATE:HIGH "+str(Vmax)+"V")

def SendTrigger():
    print("FunctionGeneratorPython: Sending trigger")
    instrument.write("*TRG")
