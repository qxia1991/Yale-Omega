
import ROOT
import datetime, time

if ROOT.gSystem.Load("libEXOUtilities") < 0:
    raise ValueError("Please set environmental variables for EXO offline")

# We need EXOFitter to load workspace to get livetime between times
if ROOT.gSystem.Load("libEXOFitting") < 0:
    raise ValueError("Please set environmental variables for EXO Fitting")
workspaceFileName = ''
exoFitter = None # Create EXOFitter as neeeded if workspace name changes
exoSourceInfo = ROOT.EXOSourceInfo()

def GetSourceActivity(sourceName,sourceStrength,startTime,endTime):
    sourceType = {}
    sourceType['Th-228'] = {'weak':ROOT.EXOSourceInfo.Weak_228Th,'strong':ROOT.EXOSourceInfo.Strong_228Th}
    sourceType['Co-60'] = {'weak':ROOT.EXOSourceInfo.Weak_60Co,'strong':ROOT.EXOSourceInfo.Strong_60Co}
    sourceType['Cs-137'] = {'weak':ROOT.EXOSourceInfo.Weak_137Cs}
    sourceType['Ra-226'] = {'weak':ROOT.EXOSourceInfo.Weak_226Ra}

    integratedActivity = exoSourceInfo.GetIntegratedActivity(sourceType[sourceName][sourceStrength],startTime,endTime)
    deltaTime = 1.*(endTime - startTime)
    if deltaTime > 0:
        averageActivity = integratedActivity*1./deltaTime
    else:
        averageActivity = 0.
    #print sourceName,sourceStrength,startTime,endTime,sourceType,integratedActivity
    return integratedActivity, averageActivity

def GetLivetimeFromWorkspace(workspace,startTime,endTime):
    global exoFitter
    global workspaceFileName

    if exoFitter == None or workspace != workspaceFileName:
    
        if exoFitter:
            exoFitter.Delete()

        workspaceFileName = workspace
        workspaceFile = ROOT.TFile.Open(workspaceFileName)
        workspaceName = workspaceFile.Get('fWorkspaceName')
        workspaceName = workspaceName.GetTitle()
        workspaceFile.Close()
    
        exoFitter = ROOT.EXODataSetModule()
        if exoFitter.LoadWspFile(workspaceFileName,workspaceName) < 0:
            exoFitter.Delete()
            exoFitter = None
            workspaceFileName = ''
            raise ValueError("Problem to load workspace %s" % fullWorkspaceFileName)
    
    return exoFitter.GetLivetimeBetween(startTime,endTime)

# You will need python-mysqldb installed to use the MySQLdb module.
# This database will have information like prescale, but I haven't had a chance yet to work on it.
import MySQLdb
daqdb_connection = None # Form connection in a lazy way -- only as needed.
def MakeDAQConnection():
    """Only do this when we know it is needed."""
    global daqdb_connection
    if daqdb_connection == None:
        daqdb_connection = MySQLdb.connect(host = "exodb01.slac.stanford.edu",
                                           port = 3606,
                                           user = "online",
                                           passwd = "exo_online",
                                           db = "exoddb")

# Create week start times only once if needed
weekIndexStartTime = None
def MakeWeekIndexStartTime(isLJdef = True, forceNewList = False):
    
    """Create list of week index and associated start times.

    Note that all times are in UTC.
    As possible, we avoid handling times directly by calling GetStartTimeOfRun.
    """

    global weekIndexStartTime
    if weekIndexStartTime == None or forceNewList:
        beforeTime = ROOT.TTimeStamp(2000, 1, 1, 0, 0, 0, 0, True) #all times before weekly calibration is applied
        startTime = ROOT.TTimeStamp(2011, 9, 16, 11, 0, 0, 0, True) #first time when weekly calibration becomes applied
        splitTime1 = ROOT.TTimeStamp(2011, 9, 21, 3, 0, 0, 0, True) #time split due to New U-wires shaping time
        splitTime2 = ROOT.TTimeStamp(2011, 9, 28, 16, 0, 0, 0, True) #time split due to new APD bias voltage, run 2401
        splitTime3 = ROOT.TTimeStamp(2011, 10, 1, 1, 13, 0, 0, True) #time split due to new APD bias voltage, run 2424
        splitTime4 = ROOT.TTimeStamp(2011, 10, 9, 18, 0, 0, 0, True) #time split adjustment, run 2473, Cs run
        lastTime = ROOT.TTimeStamp (2019, 12, 31, 0, 0, 0, 0, True) #last time in list

        weekIndexStartTime = []
        weekIndexStartTime.append(beforeTime.GetSec())
        weekIndexStartTime.append(startTime.GetSec())
        weekIndexStartTime.append(splitTime1.GetSec())
        weekIndexStartTime.append(splitTime2.GetSec())
        weekIndexStartTime.append(splitTime3.GetSec())
        weekIndexStartTime.append(splitTime4.GetSec())

        if isLJdef:
            for weekTimeSec in range(startTime.GetSec() + (len(weekIndexStartTime)-1)*86400*7, lastTime.GetSec(), 86400*7):
                weekIndexStartTime.append(weekTimeSec)
        else:
            for weekTimeSec in range(startTime.GetSec() + (len(weekIndexStartTime)-1)*86400*7, lastTime.GetSec(), 86400*7):
                weekIndexStartTime.append(weekTimeSec)            

        weekIndexStartTime.append(lastTime.GetSec())

# Create noise start times only once if needed
noiseIndexStartTime = None
def MakeNoiseIndexStartTime(forceNewList = False):

    """Create list of noise index and associated start times.

    Note that all times are in UTC.
    As possible, we avoid handling times directly by calling GetStartTimeOfRun.
    """

    global noiseIndexStartTime
    if noiseIndexStartTime == None or forceNewList:

        beforeTime = ROOT.TTimeStamp(2000, 1, 1, 0, 0, 0, 0, True)   #all times before weekly calibration is applied
        splitTime0 = ROOT.TTimeStamp(2011, 9, 18, 8, 50, 7, 0, True)   #first P1 source run (2315) is 09-18-2011
        splitTime1 = ROOT.TTimeStamp(2012, 5, 13, 6, 52, 50, 0, True) #run 3709
        splitTime2 = ROOT.TTimeStamp(2012, 10, 31, 12, 30, 3, 0, True)  #run 4399
        splitTime3 = ROOT.TTimeStamp(2013, 6, 7, 11, 34, 54, 0, True)  #run 5198
        splitTime4 = ROOT.TTimeStamp(2013, 9, 1, 10, 40, 0, 0, True)  #run 5600
        splitTime5 = ROOT.TTimeStamp(2013, 11, 3, 8, 3, 35, 0, True)  #run 5871
        splitTime6 = ROOT.TTimeStamp(2014, 2, 25, 16, 43, 53, 0, True)  #run 6371
        lastTime = ROOT.TTimeStamp (2019, 12, 31, 0, 0, 0, 0, True)  #last time in list

        noiseIndexStartTime = []
        noiseIndexStartTime.append(beforeTime.GetSec())
        noiseIndexStartTime.append(splitTime0.GetSec())
        noiseIndexStartTime.append(splitTime1.GetSec())
        noiseIndexStartTime.append(splitTime2.GetSec())
        noiseIndexStartTime.append(splitTime3.GetSec())
        noiseIndexStartTime.append(splitTime4.GetSec())
        noiseIndexStartTime.append(splitTime5.GetSec())
        noiseIndexStartTime.append(splitTime6.GetSec())
        noiseIndexStartTime.append(lastTime.GetSec())


###############################################################################
# User functions -- generally you'll only call these.                         #
###############################################################################

def GetTypeOfRun(runNo):
    """Return the type of the run.

    For example, a source run returns "Data-Source calibration"
    """
    runType = ''
    try:
        runInfo = GetRunInfo(runNo)
        runType = runInfo.FindMetaData("runType").AsString()
    except:
        pass
    
    return runType
    
def GetCommentsOfRun(runNo):
    """Return comments of the run."""

    runInfo = GetRunInfo(runNo)
    commentMeta = runInfo.FindMetaData("comment")
    if not commentMeta:
        return ''
    return commentMeta.AsString()

def GetExposureOfRun(runNo):
    """Return exposure of the run."""

    runInfo = GetRunInfo(runNo)
    return runInfo.FindMetaData("exposure").AsString()

def GetQualityOfRun(runNo):
    """Return quality of the run."""

    runInfo = GetRunInfo(runNo)
    return runInfo.FindMetaData("quality").AsString()

def GetTimeIntervalOfRun(runNo):
    """Return duration of the run."""

    startTime = GetStartTimeOfRun(runNo)
    endTime = GetEndTimeOfRun(runNo)
    startTime = GetSec(startTime)
    endTime = GetSec(endTime)

    return endTime - startTime

def GetNominalSourceLocationOfRun(runNo):
    """Return the nominal source position.  Throw ValueError if runNo is not a source run.

    For example, S5 will return the string: "S5: P4_px (  25.4,   0.0,   0.0)"
    (The string is taken directly from the restful interface; the numbers are in centimeters.)
    """
    runType = GetTypeOfRun(runNo)
    if runType != "Data-Source calibration":
        raise ValueError("Run %i has type %s; it is not a source run." % (runNo, runType))

    return GetRunInfo(runNo).FindMetaData("sourcePosition").AsString()

def GetNominalSourceLocationSOfRun(runNo):
    """Return the number of the location, e.g. at S5 returns 5.
    New S5 and old S5 are the same, so the distinction is left to GetSourcePosition function.
    """

    # run found by Raymond
    if runNo == 2423:
        return 2

    # study run for Liangjian, position seems to vay as described in run comments
    if runNo == 4884: 
        return 0

    # run found by Josh
    if runNo == 5058:
        return 5

    # poorly label run, well commented in RunSummary
    if runNo == 5418:
        return 5

    # Ra226 runs at new S5 are labeled as 'Other position'
    if runNo in range(5810,5818):
        return 5

    nominalPosition = GetNominalSourceLocationOfRun(runNo)
    positionS = nominalPosition[1:nominalPosition.find(":")]
    if not positionS.isdigit():
        return 0
    
    return int(positionS)

def GetSourcePositionOfRun(runNo):
    """Return the source position (x,y,z) that should be closest to simulated in MC.
    For now, uses most common MC locations for locations S2, S5, S8 and S11. 
    No simulation for S17, so use nominal: (0,-25.4,0).
    For now, only checks comments to decide between new and old S5.
    """

    positionS = GetNominalSourceLocationSOfRun(runNo)
    if positionS == 2:
        return -2.5, 0.23, -29.2

    if positionS == 5 and runNo > 6370:
        return 25.5, 0.39, 0.0
    elif positionS == 5:
        comments = GetCommentsOfRun(runNo)
        comments = comments.lower()


        zPos = -3.

        if comments.find('s5') >= 0 and runNo >= 5045:
            zPos = 0.
            if (comments.find('new')>=0 and comments.find('position')) or ((comments.find('+')>=0) and (comments.find('1')>=0 or comments.find('inch')>=0 or comments.find("''")>=0)):
                zPos = 0. #return 25.5, 0.39, 0
            if comments.find('old') >= 0:
                zPos = -3. #return 25.5, 0.39, -3.

        try:
            zPos = GetZPositionFromTrendingDBOfRun(runNo)/10.
        except:
            pass

        if zPos < -1.:
            zPos = -3.
        else:
            zPos = 0.

        return 25.5, 0.39, zPos

    if positionS == 8:
        return 2.5, 0.23, 29.5

    if positionS == 11:
        return 0.4, 25.5, 1.8

    if positionS == 17:
        return 0., -25.4, 0.

    return 0., 0., 0.   
    

def GetComptonSourceLocationOfRun(runNo):
    """Return the source position reported by the compton script.

    Throw ValueError if that run has no compton position.
    The return value is a dictionary: {'x' : xpos, 'y' : ypos, 'z' : zpos}, with positions in mm.
    """

    # First verify it's a source run, to avoid unnecessary queries to the database.
    runType = GetTypeOfRun(runNo)
    if runType != "Data-Source calibration":
        raise ValueError("Run %i has type %s; it is not a source run." % (runNo, runType))

    MakeDAQConnection() # Create connection if necessary.
    cursor = daqdb_connection.cursor()
    cursor.execute('SELECT path, value FROM offlineTrending ' +
                   'WHERE runIndex = %s AND path LIKE "SourcePositionSA/%%"', str(runNo))
    if cursor.rowcount != 3:
        raise MySQLdb.DataError('We found %i rows for run %i; expected exactly three.'
                                % (cursor.rowcount, runNo))
    pos = {}
    for i in range(3):
        row = cursor.fetchone()
        pos[row[0][-1]] = float(row[1])
    return pos

def GetZPositionFromTrendingDBOfRun(runNo):
    """Return the Z position measured for a run from trending DB"""

    # We really hate to query the daq database if not necessary.
    # First check that it's a thorium source run.
    if (GetTypeOfRun(runNo) != "Data-Source calibration"):
        raise ValueError("Run %i is not a source run." % runNo)

    # OK, it's a thorium source run; get the purity.
    MakeDAQConnection() # Create connection if necessary.
    cursor = daqdb_connection.cursor()
    cursor.execute('SELECT value FROM offlineTrending ' +
                   'WHERE runIndex = %s AND path = "TPCEventRatio/Zposition"', str(runNo))
    if cursor.rowcount != 1:
        raise MySQLdb.DataError('We found %i rows for run %i; expected exactly one.'
                                % (cursor.rowcount, runNo))
    return float(cursor.fetchone()[0])


def GetSourceTypeOfRun(runNo):
    """Return the type of the source used.  Throw ValueError if runNo is not a source run.

    For example, we may return "Th-228:weak"
    """
    runType = GetTypeOfRun(runNo)
    if runType != "Data-Source calibration":
        raise ValueError("Run %i has type %s; it is not a source run." % (runNo, runType))

    return GetRunInfo(runNo).FindMetaData("sourceType").AsString()

def GetSourceNameOfRun(runNo):
    
    sourceType = GetSourceTypeOfRun(runNo)
    sourceName = sourceType[:sourceType.rfind(':')]

    if runNo== 4435:
        sourceName = 'Cs-137'

    return sourceName

def GetSourceStrengthOfRun(runNo):

    sourceType = GetSourceTypeOfRun(runNo)
    sourceStrength = sourceType[sourceType.rfind(':')+1:]

    return sourceStrength
    

def GetStartTimeOfRun(runNo):
    """Return the datetime object corresponding to the start of a run.

    Python isn't terribly helpful with timezones; the returned datetime is
    a timezone-naive object in UTC.  So, just always work in UTC and you'll be happier
    """
    runInfo = GetRunInfo(runNo)
    startTimeString = runInfo.FindMetaData("startTime").AsString()
    startTimeString = startTimeString[:-5] # It's actually returned in UTC, time zone is red herring.
    startTimeString = startTimeString[:-4] # Millisecond-precision times aren't actually saved to database.

    #if hasattr(datetime.datetime, 'strptime'):
    #    #new python
    #    strptime = datetime.datetime
    #else:
    strptime = lambda date_string, format: datetime.datetime(*(time.strptime(date_string,format)[0:6]))

    #startTime = datetime.datetime.strptime(startTimeString, "%Y-%m-%dT%H:%M:%S")
    startTime = strptime(startTimeString, "%Y-%m-%dT%H:%M:%S")
    return startTime

def GetEndTimeOfRun(runNo):
    """Return the datetime object corresponding to the end of a run.

    Python isn't terribly helpful with timezones; the returned datetime is
    a timezone-naive object in UTC.  So, just always work in UTC and you'll be happier
    """
    runInfo = GetRunInfo(runNo)
    endTimeString = runInfo.FindMetaData("endTime").AsString()
    endTimeString = endTimeString[:-5] # It's actually returned in UTC, time zone is red herring.
    endTimeString = endTimeString[:-4] # Millisecond-precision times aren't actually saved to database.

    #if hasattr(datetime.datetime, 'strptime'):
    #    #new python
    #    strptime = datetime.datetime
    #else:
    strptime = lambda date_string, format: datetime.datetime(*(time.strptime(date_string,format)[0:6]))

    #endTime = datetime.datetime.strptime(endTimeString, "%Y-%m-%dT%H:%M:%S")
    endTime = strptime(endTimeString, "%Y-%m-%dT%H:%M:%S")
    return endTime



def GetWeekOfRun(runNo):
    """Return the week index of this run."""
    runNo = int(runNo)
    runTime = GetStartTimeOfRun(runNo)
    return GetWeekOfDate(runTime)

def GetPurityFromTrendingDBOfRun(runNo):
    """Return the purity measured for a run from trending DB; 
    NOT evaluating the polynomial fit, but actual measured purity using FinishUp scripts.

    Purity will be in microseconds."""

    # We really hate to query the daq database if not necessary.
    # First check that it's a thorium source run.
    if (GetTypeOfRun(runNo) != "Data-Source calibration"):
        raise ValueError("Run %i is not a source run." % runNo)

    # OK, it's a thorium source run; get the purity.
    MakeDAQConnection() # Create connection if necessary.
    cursor = daqdb_connection.cursor()
    cursor.execute('SELECT value FROM offlineTrending ' +
                   'WHERE runIndex = %s AND path = "ElectronLifetimeAR/tau"', str(runNo))
    if cursor.rowcount != 1:
        raise MySQLdb.DataError('We found %i rows for run %i; expected exactly one.'
                                % (cursor.rowcount, runNo))
    return float(cursor.fetchone()[0])

def GetPurityOfRun(runNo):
    """Try to get purity evaluated for individual run, else simply returns useless value of 0.
    Purity will be in miliseconds."""

    purity = 0.
    try:
        purity = GetPurityFromTrendingDBOfRun(runNo) / 1.e3
    except:
        pass
    
    return purity
    
def GetPurityInDB(runNo):
    """Try to get purity evaluated for individual run, else simply returns useless value of 0.
    Purity will be in miliseconds."""
    
    runTime = (GetSec(GetStartTimeOfRun(runNo)) + GetSec(GetEndTimeOfRun(runNo)))//2
    purity = 0.
    try:
        if runNo < 6376:
            flavor1, flavor2 = 'MC-v2-TPC1', 'MC-v2-TPC2'
        else:
            flavor1, flavor2 = 'MC-2D-v2-TPC', 'MC-2D-v2-TPC'
            
        eldb1 =  ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOLifetimeCalibHandler.GetHandlerName(),flavor1,runTime)
        eldb2 =  ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOLifetimeCalibHandler.GetHandlerName(),flavor2,runTime)
        purity = (eldb1.lifetime(runTime)+eldb2.lifetime(runTime))/2.e6
    except:
        pass
    
    return purity

def GetManualFlagOfRun(runNo):
    """Specific flags of runs that are not explicitly stored anywhere else."""
    
    flag = 'NONE'
    
    # some runs that have been flagged bad manually
    badRuns = []
    # strange bump near Tl characteristic peak
    badRuns.append(3856)
    # known poor purity period, purity recovering shape
    for r in range(5338,5360):
        badRuns.append(r)
    # strange values in Sean's purity file
    badRuns.append(4136)
    for r in range(4151,4158):
        badRuns.append(r)

    # strange spectrum
    badRuns.append(7490)

    # some runs are known to belong to low field campaign
    lowField = [3790,4101,4102,4104,4105,4106,4111,4112,4113,4116]

    # little or no data
    noData = [2761,3417,3418,3419,5014,5015,5016,5418,7248,7633,7640]

    # offset after weekly fit
    offsetFit = [5543,5546,5549,5589,5598,4206,4232,4341,5013]

    if runNo in badRuns:
        flag = 'BAD'
    
    if runNo in lowField:
        flag = 'LOW-FIELD'

    if runNo in noData:
        flag = 'LITTLE-OR-NO-DATA'
    
    if runNo in offsetFit:
        flag = 'OFFSET-AFTER-WEEKLY-FIT'    

    return flag  

    
'''
def GetTriggerOfRun(runNo):
    """Get the trigger information for runNo.

    Return a dictionary with keys:
      uwire_individual
      uwire_sum
      vwire_individual
      vwire_sum
      apd_individual
      apd_sum
      solicited
    Only the keys which correspond to triggers actually employed will be included.
    The value from each key will be a dictionary to properties
'''
###############################################################################
# Utility functions -- usually you wouldn't call these (though you can).      #
###############################################################################

def GetSec(time):

    time = ROOT.TTimeStamp(time.year,time.month,time.day,time.hour,time.minute,time.second,0,True)

    return time.GetSec()

def GetWeekOfDate(time):
    """Input should be a datetime object, eg. from GetStartTimeOfRun.  Returns a week index."""

    global weekIndexStartTime
    MakeWeekIndexStartTime()

    sec = GetSec(time)
    if sec > weekIndexStartTime[0]:
        for i in range(1, len(weekIndexStartTime)):
            if sec > weekIndexStartTime[i-1] and sec < weekIndexStartTime[i]:
                return i-1
            
    return -1

def GetCampaignOfRun(runNo):
    """Returns calibration campaign index as divided by Liangjian"""

    week = GetWeekOfRun(runNo)
    # group campaigns with Co and Cs runs
    if week in range(1,10):#week == 3:
        return 1 
    if week in range(10,38):#week == 27:
        return 2
    if week in range(38,55):#week == 49:
        return 3
    if week in range(55,68):#week == 61:
        return 4
    if week in range(68,81):#week == 75:
        return 5
    if week in range(81,90):#week == 87:
        return 6
    if week in range(90,93):#week == 92:
        return 7
    if week in range(93,104):#week == 93:
        return 8
    if week in range(104,116):#week == 111:
        return 9
    if week in range(116,126):#if week >= 116:#week == 120:
        return 10
    if week in range(126,200):#WIPP shut down break
        return 11
    if week >= 229:#back of operations
        return 12    

    return 0

def GetNoisePeriod(runNo):
    # Get which constant noise period to use for this run
    # these where determined in March 2017 using APD noise only 
    # (although similar trends for wires were observed).  
    # There are 6 periods for Phase 1  and only one period for
    
    if runNo < 2315:
        #Pre Phase 1 data
        return 0
    elif runNo >= 2315 and runNo < 3709:
        return 1
    elif runNo >=3709 and runNo < 4399:
        return 2
    elif runNo >= 4399 and runNo < 5198:
        return 3
    elif runNo >= 5198 and runNo <5600:
        return 4
    elif runNo >= 5600 and runNo < 5871:
        return 5
    elif runNo >= 5871 and runNo < 6371:
        return 6
    else:
        # Phase 2 data after 6370
        return 7


def GetCampaignLivetime(runNo,workspaceName):

    global weekIndexStartTime
    MakeWeekIndexStartTime()

    cpgNo = GetCampaignOfRun(runNo)
    if cpgNo == 0:
        return 0.
    if cpgNo == 1: 
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[1],weekIndexStartTime[10])
    if cpgNo == 2:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[10],weekIndexStartTime[38])
        #return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[1],weekIndexStartTime[38])
    if cpgNo == 3:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[38],weekIndexStartTime[55])
    if cpgNo == 4:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[55],weekIndexStartTime[68])
    if cpgNo == 5:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[68],weekIndexStartTime[81])
    if cpgNo == 6:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[81],weekIndexStartTime[90])
    if cpgNo == 7:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[90],weekIndexStartTime[93])
    if cpgNo == 8:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[93],weekIndexStartTime[104])
    if cpgNo == 9:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[104],weekIndexStartTime[116])
    if cpgNo == 10:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[116],weekIndexStartTime[126])
    if cpgNo == 11:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[126],weekIndexStartTime[200])
    if cpgNo == 12:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[229],weekIndexStartTime[len(weekIndexStartTime)-1])
    
    raise ValueError("Invalid LJ campaign index %s" % str(cpgNo))

def GetNoiseLivetime(runNo,workspaceName):
    global noiseIndexStartTime
    MakeNoiseIndexStartTime()
   
    noisePeriod = GetNoisePeriod(runNo)

    return  GetLivetimeFromWorkspace(workspaceName, noiseIndexStartTime[noisePeriod], noiseIndexStartTime[noisePeriod+1])

def GetLJCampaignOfRun(runNo):
    """Returns calibration campaign index as divided by Liangjian"""

    week = GetWeekOfRun(runNo)
    # group campaigns with Co and Cs runs
    if week == 3:
        return 1 
    if week == 4:
        return 1
    if week == 27:
        return 1
    if week == 61:
        return 2
    if week == 75:
        return 3
    if week == 87:
        return 4
    if week == 92:
        return 5
    if week == 93:
        return 5
    if week == 111:
        return 6

    return 0

def GetLJCampaignLivetime(runNo,workspaceName):

    global weekIndexStartTime
    MakeWeekIndexStartTime()

    cpgNo = GetLJCampaignOfRun(runNo)
    if cpgNo == 0:
        return 0.
    if cpgNo == 1: 
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[1],weekIndexStartTime[44])
    if cpgNo == 2:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[44],weekIndexStartTime[68])
    if cpgNo == 3:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[68],weekIndexStartTime[81])
    if cpgNo == 4:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[81],weekIndexStartTime[90])
    if cpgNo == 5:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[90],weekIndexStartTime[104])
    if cpgNo == 6:
        return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[104],weekIndexStartTime[len(weekIndexStartTime)-1])
    
    raise ValueError("Invalid LJ campaign index %s" % str(cpgNo))

def GetLivetimeAssociatedToWeek(weekIndex,workspaceName):
    """Get livetime between start and end of a given week index for a, also given, analysis in form of a workspace"""
    
    # then get list of week index start times and check if week index exists
    global weekIndexStartTime
    MakeWeekIndexStartTime()

    if weekIndex < 0 or weekIndex >= len(weekIndexStartTime)-1:
        raise ValueError("Invalid week index %s" % str(week))
    
    return GetLivetimeFromWorkspace(workspaceName,weekIndexStartTime[weekIndex],weekIndexStartTime[weekIndex+1])
    
def GetPhysicsTriggerFileOfRun(runNo):
    """Get the trigger configuration file as a string.  Needs to be parsed as XML."""
    runNo = int(runNo)

    MakeDAQConnection() # Create connection if necessary.
    cursor = daqdb_connection.cursor()
    cursor.execute('SELECT configFile.file ' +
                   'FROM runConfig LEFT JOIN configFile ' +
                   'ON runConfig.configInstance = configFile.configFile ' +
                   'WHERE runConfig.configType = 0 AND runConfig.configIndex = 0 ' +
                   'AND runConfig.runIndex = %s', str(runNo))
    if cursor.rowcount != 1:
        raise MySQLdb.DataError(('We found %i rows for run %i; expected exactly one.\n' +
                                 'Probably this run was not a physics-trigger run.\n' +
                                 '\t(Ie noise, laser, charge injection, which are handled differently.)')
                                % (cursor.rowcount, runNo))
    return cursor.fetchone()[0]

def GetTriggerPrescaleOfRun(runNo):
     """Get the trigger prescale from configuration file as a string."""

     prescale = {'Weak Th Prescale 1':1, 'Calibration VG Individual pos S2':10, 'Weak Th test script':10, 'Strong Co 50Hz':-50, 'Source Calibration - Prescale 2':2, 'Source Calibration - Prescale 3':3, 'Source Calibration - Prescale 1':1, 'Source Calibration - Prescale 4':4, 'APD sum':10, 'Strong Th 50Hz':-50, 'Weak Ra - No prescale':0, 'Background Run May 11 2011':10, 'Weak Co - No prescale':0, 'Physics Run':0, 'SourceCalibration-Prescale1_1630':1, 'Calibration VG Ind no prescale':0, 'Weak Cs Prescale 1':1, 'test script':10, 'Weak Th Prescale 1-APD Trouble':1, 'Weak Th - No prescale':0, 'Calibration 50Hz':-50, 'Calib VG Ind (1Hz rand trig)':-1, 'Calibration 20Hz':-20, 'SourceCalibration-Prescale1_1620':1, 'Calibration: APD sum':10, 'Calibration VG Individual':10, 'Weak Co Prescale 1':1, 'SourceCalibration-Prescale1_1640':1}
    
     trigger = 999
     try:
         triggerBody = GetPhysicsTriggerFileOfRun(runNo)
         triggerBodyList = triggerBody.split('"')

         isTrigger = False
         for content in triggerBodyList:
             if isTrigger:
                 trigger = content
                 break
             if content.find('EXO_PhysicsTrigger') > 0:
                 isTrigger = True

         trigger = prescale[trigger]
     except:
         pass
     
     return trigger

def GetHighVoltageFileOfRun(runNo):
    """Get the HV configuration file as a string.  Needs to be parsed as XML."""
    runNo = int(runNo)

    MakeDAQConnection() # Create connection if necessary.
    cursor = daqdb_connection.cursor()
    cursor.execute('SELECT configFile.file ' +
                   'FROM runConfig LEFT JOIN configFile ' +
                   'ON runConfig.configInstance = configFile.configFile ' +
                   'WHERE runConfig.configType = 0 AND runConfig.configIndex = 1 ' +
                   'AND runConfig.runIndex = %s', str(runNo))
    if cursor.rowcount != 1:
        raise MySQLdb.DataError(('We found %i rows for run %i; expected exactly one.\n' +
                                 'Probably this run was not a HV run.\n' +
                                 '\t(Ie noise, laser, charge injection, which are handled differently.)')
                                % (cursor.rowcount, runNo))
    return cursor.fetchone()[0]

def GetHighVoltageOfRun(runNo):
     """Get the HV from configuration file as a string."""

     hvs = {'All off':0}
     hvs['609V-NewAPDBias-02-12-16'] = 0.609
     hvs['1000V-NewAPDBias-02-12-16'] = 1
     hvs['1749V-NewAPDBias-02-12-16'] = 1.749
     hvs['4069V-NewAPDBias-02-12-16'] = 4.069
     hvs['8kV-NewAPDBias-04-13-16'] = 8
     hvs['8kV-NewAPDBias-08-16-11'] = 8
     hvs['8kV-NewAPDBias-09-08-11'] = 8
     hvs['8kV-NewAPDBias-09-28-11'] = 8
     hvs['8kV-NewAPDBias-09-30-11'] = 8
     hvs['8kV-APD-North-Prob-02-12-12'] = 8
     hvs['12000V-NewAPDBias-04-16'] = 12
     hvs['13000V-NewAPDBias-02-25-16'] = 13
     hvs['13500V-NewAPDBias-02-25-16'] = 13.5
     hvs['13980V-NewAPDBias-02-25-16'] = 13.98

     hv = 0
     try:
         hvBody = GetHighVoltageFileOfRun(runNo)
         hvBodyList = hvBody.split('"')

         isHV = False
         for content in hvBodyList:
             if isHV:
                 hv = content
                 break
             if content.find('EXO_HighVoltage') > 0:
                 isHV = True

         if hv in hvs:
             hv = hvs[hv]
     except:
         pass
     
     return hv

def GetRunInfo(runNo):
    """Get the EXORunInfo object for a run."""
    runNo = int(runNo)
    runInfo = ROOT.EXORunInfoManager.GetDataRunInfo(runNo)
    if not isinstance(runInfo, ROOT.EXODataRunInfo) or runInfo.GetRunNumber() == 0:
        raise MySQLdb.DataError('Run %i is not catalogued in the data catalog.' % runNo)
    return runInfo
