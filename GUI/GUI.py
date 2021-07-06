import wx
import wx.lib.scrolledpanel
import wx.lib.newevent
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.patches as patches
#from matplotlib.backends.backend_wx import NavigationToolbar2Wx
#matplotlib.use('WXAgg')
#from wx.lib.masked import NumCtrl
import math
import itertools
import numpy as np
import zmq
import json
import msgpack


PlotEvent, EVT_PLOTEVENT = wx.lib.newevent.NewEvent()

class PlotPanel(wx.Panel):
    def __init__(self, parent, title="", xlabel="", ylabel="", xlimits=[0,1], ylimits=[0,1]):
        wx.Panel.__init__(self, parent)

        self.xlabel = xlabel
        self.ylabel = ylabel
        self.xlimits = xlimits
        self.ylimits = ylimits

        self.figure = Figure()
        self.figure.subplots_adjust(left=0.16, bottom=0.14)
        self.figure.set_size_inches(8/1.75,6/1.75)

        self.axes = self.figure.add_subplot(1, 1, 1)
        self.axes.set_title(title)
        self.axes.set_xlabel(self.xlabel)
        self.axes.set_ylabel(self.ylabel)
        self.line, = self.axes.plot([0. for x in range(10000)], animated=True)
        self.axes.set_xlim(self.xlimits)
        self.axes.set_ylim(self.ylimits)
        self.canvas = FigureCanvas(self, -1, self.figure)
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.canvas, 1, wx.LEFT | wx.TOP | wx.GROW)
        self.SetSizer(self.sizer)
        self.background = self.canvas.copy_from_bbox(self.axes.bbox)
        self.Fit()

    def drawEvent(self, event):
        if event.waveform is None:
           return
        self.draw(event.waveform)

    def draw(self, waveform):
        self.axes.clear()
        self.axes.set_xlabel(self.xlabel)
        self.axes.set_ylabel(self.ylabel)
        self.axes.set_xlim(self.xlimits)
        self.axes.set_ylim(self.ylimits)
        self.axes.plot(waveform)
        self.canvas.draw()
        self.Update()


class HeatmapPanel(wx.Panel):
    def __init__(self, parent, title="", xlabel="", ylabel="", zlabel=""):
        wx.Panel.__init__(self, parent)

        self.parent = parent
        self.zlabel = zlabel

        self.figure = Figure()
        self.figure.subplots_adjust(left=0.16, bottom=0.14)
        self.figure.set_size_inches(8/1.75,6/1.75)

        self.axes = self.figure.add_subplot(1,1,1)
        self.axes.set_title(title)
        self.axes.set_xlabel(xlabel)
        self.axes.set_ylabel(ylabel)

        self.canvas = FigureCanvas(self, -1, self.figure)
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.canvas, 1, wx.LEFT | wx.TOP | wx.GROW)
        self.SetSizer(self.sizer)
        #self.Fit()

        self.resizeGrid([0,1], [0,1], 1, 1)

        self.cid = self.canvas.mpl_connect('button_press_event', self)

    def resizeGrid(self, rangeX, rangeY, stepSizeX, stepSizeY):
        self.stepSizeX = stepSizeX
        self.stepSizeY = stepSizeY

        nGridX = math.ceil(abs(rangeX[1]-rangeX[0])/stepSizeX)+1
        nGridY = math.ceil(abs(rangeY[1]-rangeY[0])/stepSizeY)+1
        self.peakArray = np.zeros((nGridY, nGridX))
        self.waveformArray = np.ndarray(shape=(nGridX,nGridY),dtype=object)

        self.xcoords = np.array([rangeX[0] + i*stepSizeX for i in range(nGridX)])
        self.ycoords = np.array([rangeY[0] + i*stepSizeY for i in range(nGridY)])

        plotranges = [self.xcoords[0]-stepSizeX/2., self.xcoords[-1]+stepSizeX/2.,\
                      self.ycoords[0]-stepSizeY/2., self.ycoords[-1]+stepSizeY/2.]       

        self.image = self.axes.imshow(self.peakArray, cmap="RdBu",\
                                      extent=plotranges, origin="lower", aspect="auto")

        if not hasattr(self, 'cbar'):
            self.cbar = self.axes.figure.colorbar(self.image, ax=self.axes)
            self.cbar.ax.set_ylabel(self.zlabel, rotation=-90, va="bottom")

        self.canvas.draw()
        self.Update()

    def draw(self):
        self.axes.patches = []
        self.image.set_data(self.peakArray)
        self.image.set_clim(np.amin(self.peakArray), np.amax(self.peakArray))
        self.cbar.remove()
        self.cbar = self.axes.figure.colorbar(self.image, ax=self.axes)
        self.cbar.ax.set_ylabel(self.zlabel, rotation=-90, va="bottom")
        self.canvas.draw()
        self.Update()

    def addPeak(self, xval, yval, waveform):
        peakvalue = np.amin(waveform)
        x_i = (np.abs(self.xcoords - xval)).argmin()
        y_i = (np.abs(self.ycoords - yval)).argmin()
        self.peakArray[y_i, x_i] = peakvalue
        self.waveformArray[x_i, y_i] = waveform
        self.draw()

    def __call__(self,event):
        if event.inaxes != self.axes:
            return
        x_i = (np.abs(self.xcoords - event.xdata)).argmin()
        y_i = (np.abs(self.ycoords - event.ydata)).argmin()

        self.axes.patches = []
        rect = patches.Rectangle((self.xcoords[x_i]-self.stepSizeX/2, self.ycoords[y_i]-self.stepSizeY/2),\
                                 self.stepSizeX, self.stepSizeY, linewidth=2, edgecolor='g', facecolor='none')
        self.axes.add_patch(rect)
        self.canvas.draw()

        plotevent = PlotEvent(waveform=self.waveformArray[x_i, y_i])
        wx.PostEvent(self.parent, plotevent)


class RangePanel(wx.Panel):
    def __init__(self, parent, title="", height=100, rows=1):
        wx.Panel.__init__(self, parent)

        titletxt = wx.StaticText(self, label=title)
        titletxt.SetFont(wx.Font(-1, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))

        self.scrollWindow = wx.ScrolledWindow(self, size=(-1,height-25))
        self.vbox_scroll = wx.BoxSizer(wx.VERTICAL)
        self.scrollWindow.SetSizer(self.vbox_scroll)
        self.scrollWindow.SetScrollbars(0, 1, 0, 1)

        self.n_rows = 0
        for _ in range(rows):
            self.addRow(["0","0"])

        self.scrollWindow.SetSize(self.scrollWindow.GetSize()[0]+10,-1)

        button_add = wx.Button(self, wx.ID_ANY, "Add", size=(50,25))
        button_delete = wx.Button(self, wx.ID_ANY, "Delete", size=(50,25))
        self.Bind(wx.EVT_BUTTON, lambda event: self.addRow(["0","0"], event), button_add)
        self.Bind(wx.EVT_BUTTON, lambda event: self.deleteRow(False, event), button_delete)

        txt_stepsize = wx.StaticText(self, label="Step size ")
        self.field_stepsize = wx.TextCtrl(self, value="1", size=(50,20), style=wx.TE_RIGHT)

        vbox = wx.BoxSizer(wx.VERTICAL)
        vbox.Add(titletxt)
        vbox.Add(self.scrollWindow)
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(button_add)
        hbox.Add(button_delete)
        vbox.Add(hbox)
        vbox.Add(wx.StaticLine(self, style=wx.LI_HORIZONTAL), 0, wx.EXPAND|wx.ALL, 20)
        stepbox = wx.BoxSizer(wx.HORIZONTAL)
        stepbox.Add(txt_stepsize)
        stepbox.Add(self.field_stepsize)
        vbox.Add(stepbox, flag=wx.ALIGN_LEFT)

        self.SetSizer(vbox)

    def addRow(self, range, event=0):
        self.n_rows += 1
        text1 = wx.StaticText(self.scrollWindow, -1, str(self.n_rows)+": ", size=(25, -1), style=wx.ALIGN_RIGHT)
        range_start = wx.TextCtrl(self.scrollWindow, value=range[0], size=(50,20), style=wx.TE_RIGHT)
        text2 = wx.StaticText(self.scrollWindow, -1, " to ")
        range_end = wx.TextCtrl(self.scrollWindow, value=range[1], size=(50,20), style=wx.TE_RIGHT)
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.AddMany([text1, range_start, text2, range_end])
        self.vbox_scroll.Add(hbox)
        self.vbox_scroll.Layout()
        w, h = self.vbox_scroll.GetMinSize()
        self.scrollWindow.SetVirtualSize((w, h))

    def deleteRow(self, force=False, event=0):
        minrows = 0 if force else 1
        if self.n_rows > minrows:
            self.vbox_scroll.Hide(self.n_rows-1)
            self.vbox_scroll.Remove(self.n_rows-1)
            self.n_rows -= 1
            w, h = self.vbox_scroll.GetMinSize()
            self.scrollWindow.SetVirtualSize((w, h))

    def getRanges(self, event=0):
        ranges = []
        for hbox in self.vbox_scroll.GetChildren():
            txt_start = hbox.GetSizer().GetItem(1).GetWindow()
            txt_end = hbox.GetSizer().GetItem(3).GetWindow()
            ranges.append( [float(txt_start.GetValue()), float(txt_end.GetValue())] )
        return ranges

    def getRangeExtremes(self, event=0):
        allvalues = list(itertools.chain.from_iterable(self.getRanges()))
        extremes = [min(allvalues), max(allvalues)]
        return extremes

    def getStepSize(self, event=0):
        return float(self.field_stepsize.GetValue())

    def setRangesFromConfig(self, config):
        while self.n_rows > 0:
            self.deleteRow(force=True)
        for rangestr in config.split("|"):
            curr_range = rangestr.split(",")
            if len(curr_range) == 1:
                curr_range = [curr_range[0]]*2
            self.addRow(curr_range)

    def setStepSize(self, value):
        self.field_stepsize.SetValue(value)


class GUI(wx.Frame):
    def __init__(self, *args, **kwargs):
        super(GUI, self).__init__(*args, **kwargs)

        # ZMQ sockets
        self.context = zmq.Context(1)

        self.socket_pull = self.context.socket(zmq.PULL)
        self.socket_pull.bind("tcp://127.0.0.1:5555")
        self.zmqpoller = zmq.Poller()
        self.zmqpoller.register(self.socket_pull, zmq.POLLIN)

        self.socket_pub = self.context.socket(zmq.PUB)
        self.socket_pub.bind("tcp://127.0.0.1:5556")

        self.initUI()

    def initUI(self):

        self.UITimer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self._updateUI, self.UITimer)

        # Menu bar
        ####################################################

        menu_file = wx.Menu()

        file_openconfig = menu_file.Append(wx.ID_ANY, 'Open', 'Open configuration file')
        self.Bind(wx.EVT_MENU, self.openConfigFile, file_openconfig)

        file_saveconfig = menu_file.Append(wx.ID_ANY, 'Save', 'Save configuration file')
        self.Bind(wx.EVT_MENU, self.saveConfigFile, file_saveconfig)        

        file_close = menu_file.Append(wx.ID_EXIT, 'Quit', 'Quit application')
        self.Bind(wx.EVT_MENU, self.onQuit, file_close)

        menubar = wx.MenuBar()
        menubar.Append(menu_file, '&File')
        self.SetMenuBar(menubar)

        # Plots and buttons
        ####################################################

        self.plotpanel = PlotPanel(self, title="Waveforms", xlabel="Time", ylabel="Signal [V]", xlimits=[0,10000], ylimits=[-5,0])
        self.heatmappanel = HeatmapPanel(self, title="Waveform peak values",
                                         xlabel="Angle [°]", ylabel="Height [mm]", zlabel="Signal peak [V]")

        self.Bind(EVT_PLOTEVENT, self.plotpanel.drawEvent)

        self.button_start = wx.Button(self, wx.ID_ANY, "Start", size=(100,80))
        self.Bind(wx.EVT_BUTTON, self._startstopProgram, self.button_start)

        # self.textbox_angle = wx.StaticText(self, label="Current angle")
        # self.indicator_angle = wx.TextCtrl(self, style=wx.TE_READONLY)
        # self.textbox_ypos = wx.StaticText(self, label="Current y-pos")
        # self.indicator_ypos = wx.TextCtrl(self, style=wx.TE_READONLY)

        self.angleRangePanel = RangePanel(self, title="Angle ranges (°)", height=200)
        self.yRangePanel = RangePanel(self, title="Height ranges (mm)", height=200)

        txt_size = (125,20)

        self.dict_output = {
            "rootfilename"       : [wx.StaticText(self, label="File name "), wx.TextCtrl(self, size=txt_size)],
            "treename"           : [wx.StaticText(self, label="TTree name "), wx.TextCtrl(self, size=txt_size)],
            "ID_diffuser"        : [wx.StaticText(self, label="Diffuser ID "), wx.TextCtrl(self, size=txt_size)],
            "ID_PMT"             : [wx.StaticText(self, label="PMT ID "), wx.TextCtrl(self, size=txt_size)],
            "ID_PD"              : [wx.StaticText(self, label="Photodiode ID "), wx.TextCtrl(self, size=txt_size)],
            "ID_lightsource"     : [wx.StaticText(self, label="Light source ID "), wx.TextCtrl(self, size=txt_size)],
            "ID_experimentalist" : [wx.StaticText(self, label="Operator name "), wx.TextCtrl(self, size=txt_size)],
            "notes"              : [wx.StaticText(self, label="Notes "), wx.TextCtrl(self, size=(txt_size[0],100), style=wx.TE_MULTILINE)],
        }

        self.dict_funcgen = {
            "funcgen_channel"   : [wx.StaticText(self, label="Output channel "), wx.TextCtrl(self, size=txt_size)],
            "funcgen_cycles"    : [wx.StaticText(self, label="Number of cycles "), wx.TextCtrl(self, size=txt_size)],
            "funcgen_frequency" : [wx.StaticText(self, label="Frequency [Hz] "), wx.TextCtrl(self, size=txt_size)],
            "funcgen_Vmin"      : [wx.StaticText(self, label="Min output [V] "), wx.TextCtrl(self, size=txt_size)],
            "funcgen_Vmax"      : [wx.StaticText(self, label="Max output [V] "), wx.TextCtrl(self, size=txt_size)],
        }

        self.dict_devices = {
            "linmotor_devicename" : [wx.StaticText(self, label="Linear motor device name "), wx.TextCtrl(self, size=txt_size)],
            "rotmotor_USBport"    : [wx.StaticText(self, label="Angular motor USB name "), wx.TextCtrl(self, size=txt_size)],
            "humid_USBport"       : [wx.StaticText(self, label="Humidity sensor USB name "), wx.TextCtrl(self, size=txt_size)],
            "funcgen_IP"          : [wx.StaticText(self, label="Function generator IP "), wx.TextCtrl(self, size=txt_size)],
        }

        self.dict_digitizer = {
            "digitizer_numSamples"         : [wx.StaticText(self, label="Number of samples "), wx.TextCtrl(self, size=txt_size)],
            "digitizer_sampleRate"         : [wx.StaticText(self, label="Sample rate [Hz] "), wx.TextCtrl(self, size=txt_size)],
            "digitizer_triggerLevel"       : [wx.StaticText(self, label="Trigger level [V] "), wx.TextCtrl(self, size=txt_size)],
            "digitizer_inputRange"         : [wx.StaticText(self, label="Input range [V] "), wx.TextCtrl(self, size=txt_size)],
            "digitizer_inputOffsetPercent" : [wx.StaticText(self, label="Input offset [%] "), wx.TextCtrl(self, size=txt_size)],
        }

        self.dict_other = {
            "linmotor_steps_unit" : "1000",
            "rotmotor_steps_unit" : "1000",
            "rotmotor_n_motor"    : "0",
            "funcgen_shape"       : "SQUARE",
        }

        # Visual layout
        ####################################################

        vbox = wx.BoxSizer(wx.VERTICAL) # Main vertical sizer
        
        plotbox = wx.BoxSizer(wx.HORIZONTAL) # Horizontal sizer for plots
        plotbox.Add(self.plotpanel, flag=wx.LEFT)
        #plotbox.Add(self.plotpanel2, flag=wx.RIGHT)
        plotbox.Add(self.heatmappanel, flag=wx.RIGHT)

        outputbox = wx.BoxSizer(wx.VERTICAL)
        title = wx.StaticText(self, label="Output")
        title.SetFont(wx.Font(-1, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        outputbox.Add(title, flag=wx.ALIGN_RIGHT)
        for line in self.dict_output.values():
            sizer = wx.BoxSizer(wx.HORIZONTAL)
            sizer.AddMany(line)
            outputbox.Add(sizer, flag=wx.ALIGN_RIGHT)

        funcgenbox = wx.BoxSizer(wx.VERTICAL)
        title = wx.StaticText(self, label="Function generator")
        title.SetFont(wx.Font(-1, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        funcgenbox.Add(title, flag=wx.ALIGN_RIGHT)
        for line in self.dict_funcgen.values():
            sizer = wx.BoxSizer(wx.HORIZONTAL)
            sizer.AddMany(line)
            funcgenbox.Add(sizer, flag=wx.ALIGN_RIGHT)

        devicebox = wx.BoxSizer(wx.VERTICAL)
        title = wx.StaticText(self, label="Device settings")
        title.SetFont(wx.Font(-1, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        devicebox.Add(title, flag=wx.ALIGN_RIGHT)
        for line in self.dict_devices.values():
            sizer = wx.BoxSizer(wx.HORIZONTAL)
            sizer.AddMany(line)
            devicebox.Add(sizer, flag=wx.ALIGN_RIGHT)

        digitizerbox = wx.BoxSizer(wx.VERTICAL)
        title = wx.StaticText(self, label="Digitizer settings")
        title.SetFont(wx.Font(-1, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        digitizerbox.Add(title, flag=wx.ALIGN_RIGHT)
        for line in self.dict_digitizer.values():
            sizer = wx.BoxSizer(wx.HORIZONTAL)
            sizer.AddMany(line)
            digitizerbox.Add(sizer, flag=wx.ALIGN_RIGHT)

        func_device_box = wx.BoxSizer(wx.VERTICAL)
        func_device_box.Add(funcgenbox, flag=wx.ALIGN_RIGHT)
        func_device_box.Add(wx.StaticLine(self, style=wx.LI_HORIZONTAL), 0, wx.EXPAND|wx.ALL, 10)
        func_device_box.Add(devicebox, flag=wx.ALIGN_RIGHT)
        func_device_box.Add(wx.StaticLine(self, style=wx.LI_HORIZONTAL), 0, wx.EXPAND|wx.ALL, 10)
        func_device_box.Add(digitizerbox, flag=wx.ALIGN_RIGHT)        
        func_device_box.AddSpacer(10)

        rangebox = wx.BoxSizer(wx.HORIZONTAL)
        rangebox.Add(self.angleRangePanel)
        rangebox.Add(wx.StaticLine(self, style=wx.LI_VERTICAL), 0, wx.EXPAND|wx.ALL, 10)
        rangebox.Add(self.yRangePanel)

        ctrlbox = wx.BoxSizer(wx.VERTICAL) # Horizontal sizer for controls
        ctrlbox.Add(rangebox)
        ctrlbox.Add(wx.StaticLine(self, style=wx.LI_HORIZONTAL), 0, wx.EXPAND|wx.ALL, 10)
        ctrlbox.Add(self.button_start, flag=wx.BOTTOM)
        # ctrlbox.Add(self.textbox_angle)
        # ctrlbox.Add(self.indicator_angle)
        # ctrlbox.Add(self.textbox_ypos)
        # ctrlbox.Add(self.indicator_ypos)

        configbox = wx.BoxSizer(wx.HORIZONTAL)
        configbox.AddSpacer(10)
        configbox.Add(ctrlbox)
        configbox.Add(wx.StaticLine(self, style=wx.LI_VERTICAL), 0, wx.EXPAND|wx.ALL, 10)
        configbox.Add(outputbox)
        configbox.Add(wx.StaticLine(self, style=wx.LI_VERTICAL), 0, wx.EXPAND|wx.ALL, 10)
        configbox.Add(func_device_box)
        configbox.AddSpacer(10)

        vbox.Add(plotbox, flag=wx.TOP)
        vbox.AddSpacer(10)
        vbox.Add(configbox)
        #vbox.Add(wx.StaticLine(self, style=wx.LI_HORIZONTAL), 0, wx.EXPAND|wx.ALL, 10)
        #vbox.Add(ctrlbox, flag=wx.BOTTOM)

        self.SetSizerAndFit(vbox)

    def _updateUI(self, event):
        socks = dict(self.zmqpoller.poll(0))
        if self.socket_pull in socks and socks[self.socket_pull] == zmq.POLLIN:
            message = self.socket_pull.recv()
            angle, ypos, waveform_PMT, waveform_PD = msgpack.unpackb(message, use_list=False)
            waveform_PMT = np.array(waveform_PMT)
            waveform_PD = np.array(waveform_PD)
            # self.indicator_angle.SetValue(str(angle))
            # self.indicator_ypos.SetValue(str(ypos))
            self.plotpanel.draw(waveform_PMT)
            self.heatmappanel.addPeak(angle, ypos, waveform_PMT)

    def _constructConfigDict(self):
        config_dict = {}
        angleRangesStr = [[str(x[0]), str(x[1])] for x in self.angleRangePanel.getRanges()]
        yRangesStr = [[str(x[0]), str(x[1])] for x in self.yRangePanel.getRanges()]
        config_dict["rangesAngle"] = "|".join([",".join(x) for x in angleRangesStr])
        config_dict["rangesY"] = "|".join([",".join(x) for x in yRangesStr])
        config_dict["stepSizeAngle"] = str(self.angleRangePanel.getStepSize())
        config_dict["stepSizeY"] = str(self.yRangePanel.getStepSize())

        for key, value in {**self.dict_output, **self.dict_funcgen, **self.dict_devices, **self.dict_digitizer}.items():
            config_dict[key] = value[1].GetValue()

        for key, value in self.dict_other.items():
            config_dict[key] = value

        return config_dict

    def _startstopProgram(self, event):
        config_dict = self._constructConfigDict()
        self.socket_pub.send_string(json.dumps(config_dict))

        self.heatmappanel.resizeGrid(self.angleRangePanel.getRangeExtremes(),
                                     self.yRangePanel.getRangeExtremes(),
                                     self.angleRangePanel.getStepSize(),
                                     self.yRangePanel.getStepSize())

        self.UITimer.Start(100)

    def _readConfigFile(self, file):
        try:
            config = json.load(file)
        except:
            wx.LogError("Unable to parse json.")
            return

        for key, value in config.items():
            if key == "stepSizeAngle":
                self.angleRangePanel.setStepSize(value)
            elif key == "stepSizeY":
                self.yRangePanel.setStepSize(value)
            elif key == "rangesAngle":
                self.angleRangePanel.setRangesFromConfig(value)
            elif key == "rangesY":
                self.yRangePanel.setRangesFromConfig(value)
            else:
                for idict in [self.dict_output, self.dict_funcgen, self.dict_devices, self.dict_digitizer]:
                    if key in idict:
                        idict[key][1].SetValue(value)

    def openConfigFile(self, event):
        with wx.FileDialog(self, "Open config file", wildcard="config files (*.conf)|*.conf",
                       style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fileDialog:

            if fileDialog.ShowModal() == wx.ID_CANCEL: # User changed their mind
                return 

            # Proceed loading the file chosen by the user
            pathname = fileDialog.GetPath()
            try:
                with open(pathname, 'r') as file:
                    self._readConfigFile(file)
            except IOError:
                wx.LogError("Cannot open file '%s'." % file)

    def saveConfigFile(self, event):
        config_dict = self._constructConfigDict()

        with wx.FileDialog(self, "Save XYZ file", wildcard="config files (*.conf)|*.conf",
                       style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT) as fileDialog:

            if fileDialog.ShowModal() == wx.ID_CANCEL: # User changed their mind
                return 

            # save the current contents in the file
            pathname = fileDialog.GetPath()
            try:
                with open(pathname, 'w') as file:
                    json.dump(config_dict, file, indent=4)
            except IOError:
                wx.LogError("Cannot save current data in file '%s'." % pathname)

    def onQuit(self, event):
        self.Close()    


if __name__ == "__main__":

    app = wx.App()

    theGUI = GUI(None, title='Diffuser Scan', size=wx.Size(1000, 1000))
    theGUI.Show()

    app.MainLoop()
