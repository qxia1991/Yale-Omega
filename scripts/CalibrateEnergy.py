
import ROOT, settings
import sys, argparse, os

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

parser = argparse.ArgumentParser()
parser.add_argument('--week', type=str, nargs=1, default=['all'])
parser.add_argument('--channel', type=str, nargs=1, default=['Rotated'])
args = parser.parse_args()

print args, args.week, args.channel

week = args.week[0]
if week.isdigit():
    infoRuns.CutExact('WeekIndex',week)
    print 'Calibration of week', week
    weeklyFit = True
else:
    print 'No cut on weeks.'
    weeklyFit = False

# Cuts on week index
infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

# Default cuts
infoRuns.SelectDefaultRuns()
infoRuns.SelectMaxPurity()

# Additional cuts
infoRuns.CutExact('SourceName','Cs-137',False)
infoRuns.CutExact('TriggerPrescale','0',True)


# week specfic cuts
if week == '231' or week == '232':
	infoRuns.CutExact('HighVoltage','8',True) # for weeks 231 and 232 high voltage has changed

# Specify selection trees and MC files
infoRuns.SetMetadata('DataSelectionFileName',settings.SelectionTreeFileName)
infoRuns.SetMetadata('DataMultiplicityWildCard',settings.DataMultiplicityWildcard)
infoRuns.SetMetadata('DataId',settings.DataId)
infoRuns.SetMCTreeFileNames(settings.MCTreeFileName)
infoRuns.SetMCSelectionFileNames("",settings.MCSelectionFileName,settings.DataMultiplicityWildcard)

# temporary run cuts (not procesed, issues, etc) --> should be added to PolishSourceRunsInfo.py
infoRuns.CutExact('RunNumber','7536',False) # for now
infoRuns.CutExact('RunNumber','7537',False) # for now
infoRuns.CutExact('RunNumber','7490',False) # for now Th228 at S8 with strange feature
infoRuns.CutExact('RunNumber','7248',False) # for now low stat
infoRuns.CutExact('RunNumber','7633',False) # for now low stat
infoRuns.CutExact('RunNumber','7640',False) # for now low stat

# Create energy calibrater
energyCalibrator = ROOT.EXOEnergyCalibrator(infoRuns)
energyCalibrator.SetVerboseLevel(1)
energyCalibrator.SetHistoRanges(settings.FitRange[0],settings.FitRange[1],settings.FitRange[2],settings.FitRange[3])
energyCalibrator.SetInitialCalibPars(10,0.63)
energyCalibrator.SetInitialResPars(0.5,10)
energyCalibrator.SetUseAngleFromFile(False)
energyCalibrator.SetIsCalibrated(False)
if settings.useAngleFile:
	energyCalibrator.SetUseAngleFromFile(True)
	energyCalibrator.SetAngleFile(settings.angleFile)
if settings.IgnoreLimit is not None:
	energyCalibrator.SetIgnoreLimits(settings.IgnoreLimit[0],settings.IgnoreLimit[1])
        print 'Ignoring bins between ', settings.IgnoreLimit[0], ' and ', settings.IgnoreLimit[1]
channel = args.channel[0]
if channel in ['Rotated','Ionization','Scintillation']:
    print 'Calibration of channel', channel
    energyCalibrator.SetCalibrationChannel(channel)

if weeklyFit:
    infoRuns.CutExact('SourcePositionS','11',False)
    infoRuns.CutExact('SourcePositionS','2',False)
    infoRuns.CutExact('SourcePositionS','8',False)

    outResDir = settings.CalibrationOutput
    cmd = 'mkdir -p %s' % (outResDir)
    print cmd
    os.system(cmd)
    energyCalibrator.FitWeeklySources('%s/fit_week_[WEEK].root'%(outResDir),'[WEEK]')

else:
    infoRuns.CutExact('WeekIndex','001',False)
    infoRuns.CutExact('WeekIndex','002',False)
    infoRuns.CutExact('WeekIndex','003',False)
    infoRuns.CutExact('WeekIndex','004',False)
    #infoRuns.CutExact('TriggerPrescale','-50',False)

    infoRuns.CutDoubleComparison('WeekLivetime',1e-6,True)

    #energyCalibrator.FitCalibrationCampaigns('%sweighted_cpg.root'%(settings.CalibrationOutput),'%sfit_week_[WEEK].root'%(settings.CalibrationOutput),'[WEEK]')

    # anode
    infoRuns.CutExact('SourcePositionS','5',False)
    infoRuns.CutExact('SourcePositionS','11',False)

    # cathode
    # infoRuns.CutExact('SourcePositionS','2',False)
    # infoRuns.CutExact('SourcePositionS','8',False)

    energyCalibrator.FitOnlyPeaks()
    # infoRuns.CutExact('SourceName','Th-228')
    energyCalibrator.FitSpatialDistributions('%sweighted_anodepeaks.root'%(settings.CalibrationOutput),'%sfit_week_[WEEK].root'%(settings.CalibrationOutput),'[WEEK]')


