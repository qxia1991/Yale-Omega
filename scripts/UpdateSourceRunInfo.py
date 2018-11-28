
import ROOT
import PolishSourceRunInfoFromDatabases as psrifd

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

def GetRunList(minRun,maxRun):

    runList = []
    for run in range(minRun,maxRun+1):
        runList.append(run)

    return runList

def GetLJRunList():

    runList = []
    runVec = ROOT.EXOEnergyUtils.GetAllLJRunsForSource()
    if runVec:
        for idx in range(runVec.size()):
            runList.append(runVec.at(idx))

    return runList

def IsSourceRun(run):
    
    return psrifd.GetTypeOfRun(run) == "Data-Source calibration"

def FillSourceRunInfo(run,workspaceName = ''):

    runInfo = {}

    runInfo['RunNumber'] = int(run)

    runInfo['StartTime'] = psrifd.GetStartTimeOfRun(run)
    runInfo['StartTimeSec'] = psrifd.GetSec(runInfo['StartTime'])
    runInfo['EndTime'] = psrifd.GetEndTimeOfRun(run)
    runInfo['EndTimeSec'] = psrifd.GetSec(runInfo['EndTime'])

    runInfo['TimeInterval'] = psrifd.GetTimeIntervalOfRun(run)
    runInfo['Exposure'] = psrifd.GetExposureOfRun(run)
    runInfo['Quality'] = psrifd.GetQualityOfRun(run)
    runInfo['Comments'] = str(psrifd.GetCommentsOfRun(run)).replace('\n',' ')
    runInfo['TriggerPrescale'] = psrifd.GetTriggerPrescaleOfRun(run)
    runInfo['HighVoltage'] = psrifd.GetHighVoltageOfRun(run)

    runInfo['WeekIndex'] = '%03d' % psrifd.GetWeekOfRun(run)
    runInfo['CampaignIndex'] = '%02d' % psrifd.GetCampaignOfRun(run)
    runInfo['NoiseIndex'] = '%02d' % psrifd.GetNoisePeriod(run)
    #runInfo['WeekLivetime'] = 1.0
    #runInfo['CampaignLivetime'] = 1.0
    
    if not workspaceName == '':
        runInfo['WeekLivetime'] = psrifd.GetLivetimeAssociatedToWeek(int(runInfo['WeekIndex']),workspaceName)
        runInfo['CampaignLivetime'] = psrifd.GetCampaignLivetime(run,workspaceName)
        runInfo['NoiseLivetime'] = psrifd.GetNoiseLivetime(run, workspaceName)

    runInfo['SourceName'] = psrifd.GetSourceNameOfRun(run)
    runInfo['SourceZ'] = '%03d' % ROOT.EXOEnergyUtils.GetAtomicNumber(runInfo['SourceName'])
    runInfo['SourceA'] = '%03d' % ROOT.EXOEnergyUtils.GetIsotopeNumber(runInfo['SourceName'])
    runInfo['SourceStrength'] = psrifd.GetSourceStrengthOfRun(run)
    runInfo['SourcePositionS'] = psrifd.GetNominalSourceLocationSOfRun(run)
    runInfo['SourcePositionX'], runInfo['SourcePositionY'], runInfo['SourcePositionZ'] = psrifd.GetSourcePositionOfRun(run)
    runInfo['Purity'] = psrifd.GetPurityInDB(run) # as seen by LB runs
    runInfo['RunPurity'] = psrifd.GetPurityOfRun(run) # as evaluated for the source run (may not work for S2, etc)
    runInfo['ManualFlag'] = psrifd.GetManualFlagOfRun(run)

    runInfo['IntegratedActivity'], runInfo['AverageActivity'] = psrifd.GetSourceActivity(runInfo['SourceName'],runInfo['SourceStrength'],runInfo['StartTimeSec'],runInfo['EndTimeSec'])

    return runInfo

def GetInfoForRunList(runList,outputFileName = 'output.txt',workspaceName = ''):
    
    outputFile = open(outputFileName,'w')
    infoRuns = {}
    for run in runList:
        if not IsSourceRun(run):
            continue

        runInfo = FillSourceRunInfo(run,workspaceName)

        PrintRunInfo(runInfo,outputFile)
        outputLine = 'EndOfSourceRunInfo : ------------------------------------------------------'
        outputFile.write(outputLine+'\n')
        print outputLine

        infoRuns[run] = runInfo

    outputFile.close()

    return infoRuns

def PrintRunInfo(runInfo,outputFile):

    for key in sorted(runInfo.iterkeys()):
        outputLine = "%s : %s" % (key, runInfo[key])
        outputFile.write(outputLine+'\n')
        print outputLine #key, ':', runInfo[key]

    return

def GetRunListPhase(phase):

    if phase == 1:
        return GetRunList(2315,6370)
    if phase == 2:
        return GetRunList(6376,10000)
    if phase == 'phase2-lb':
        return GetRunList(7100,10000)

            
if __name__ == '__main__':

    #runList = GetRunList(6376, 8000) #phase1 = 2315 to 6370, phase2 = 6376 ->
    #runList = GetLJRunList()
    
    #runList = GetRunListPhase(2)
    #workspaceName = '../data/Run2_Denoised_20170217.wsp.root' #'EXO_Workspace_Run2abc.root'  
    #outputFileName =  '../data/UpdatedLivetimeSourceRunsInfo_Phase2_20170217.txt'

    #PhaseI from Caio. Needed to get the livetime.
    runList = GetRunListPhase('phase2-lb')
    workspaceName = '/nfs/slac/g/exo_data6/groups/Fitting/data/WIPP/preprocessed/2017_Phase2/wsps/Phase2abcd_Livetime_170426.wsp.root'
    outputFileName =  '../data/SourceRunsInfo_Phase2abcd_20170427.txt'

    infoRuns = GetInfoForRunList(runList,outputFileName,workspaceName)
        
        
