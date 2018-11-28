
import ROOT, settings
import sys, argparse

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

infoRuns.SetMetadata('DataTreeFileName',settings.DataTreeFileName)
infoRuns.SetMetadata('DataMultiplicityWildCard',settings.DataMultiplicityWildcard)
infoRuns.SetMetadata('DataId',settings.DataId)
infoRuns.SetMetadata('DataSelectionFileName',settings.SelectionTreeFileName)

infoRuns.SetMCTreeFileNames(settings.MCTreeFileName)
infoRuns.SetMCSelectionFileNames("",settings.MCSelectionFileName,settings.DataMultiplicityWildcard)

parser = argparse.ArgumentParser()
parser.add_argument('--week', type=str, nargs=1, default=['061'])
parser.add_argument('--mult', type=str, nargs=1, default=['ss'])
parser.add_argument('--energy', type=float, nargs=1, default=[2458])
parser.add_argument('--non-denoised', action="store_true", default=False)
parser.add_argument('--angle', type=float, nargs=1, default=[-1])
args = parser.parse_args()
print args

# Cuts on week index
infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

# Default cuts
infoRuns.CutDefaultRuns()

# Additional cuts
infoRuns.CutExact('SourceName','Cs-137',False)
infoRuns.CutExact('SourceName','Co-60',False)
infoRuns.CutExact('SourceName','Ra-226',False)

infoRuns.CutExact("SourcePositionS","0",False)
infoRuns.CutExact("SourcePositionS","17",False)
infoRuns.CutExact('SourcePositionS','11',False)
infoRuns.CutExact('SourcePositionS','2',False)
infoRuns.CutExact('SourcePositionS','8',False)

infoRuns.CutExact('RunNumber','6248',False)

angle = args.angle[0]

angleFitter = ROOT.EXORotationAngleFitter(args.week[0],args.mult[0],args.energy[0],infoRuns)
angleFitter.SetVerboseLevel(1)
angleFitter.SetDenoisedLight(not bool(args.non_denoised))
angleFitter.SetCalibEnergyBins(5,850,3000)
angleFitter.SaveAllAnglesResults(False)
angleFitter.UseGaussErrfc(False)
angleFitter.SetOutputFileName('%sAngleFitter_Week%s_%s.root'%(settings.AngleFitterOutput,args.week[0],args.mult[0]))
#if angleFitter.ExecuteFit("MINIMIZE"):

if angle >= 0.0:
    angleFitter.SetFitPars(False,10.0,0.6,0.6,10.0)
    angleFitter.RunFitForAngle(angle)
else:
    if angleFitter.RunScanAndFit():
        angleFitter.SaveOutputFile('%sResult_Week%s_%s_At%.1f.root'%(settings.AngleFitterOutput,args.week[0],args.mult[0],args.energy[0]),True)


